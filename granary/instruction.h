/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instruction.h
 *
 *      Author: Peter Goodman
 */

#ifndef granary_INSTRUCTION_H_
#define granary_INSTRUCTION_H_

#include <utility>

#include "granary/globals.h"
#include "granary/policy.h"

namespace granary {


    /// Forward declarations.
    struct operand;
    struct segment;
    struct operand_base_disp;
    struct operand_ref;
    struct instruction;
    struct instruction_list;


    /// Instruction decode constraints.
    enum instruction_decode_constraint {
        DECODE_WIDEN_CTI,
        DECODE_DONT_WIDEN_CTI
    };


    /// Defines a "persistent" instruction as a DR instruction.
    typedef dynamorio::instr_t persistent_instruction;


    /// Defines an operand generator for LEA operands
    struct operand_base_disp {
    public:

        typename dynamorio::reg_id_t base;
        typename dynamorio::reg_id_t index;

        unsigned char size;

        int scale;
        int disp;

        operand_base_disp(void) ;

        /// Add the scale parameter to the LEA operand. Value values are 1, 2,
        /// 4, and 8.
        template <typename T>
        operand_base_disp operator*(T scale_) const {
            static_assert(std::is_integral<T>::value,
                "Scale must have an integral type.");

            operand_base_disp ret(*this);
            ret.scale = static_cast<int>(scale_);
            return ret;
        }

        /// Add in the displacement to the LEA operand.
        template <typename T>
        operand_base_disp operator+(T disp_) const {
            static_assert(std::is_integral<T>::value,
                "Displacement must have an integral type.");

            operand_base_disp ret(*this);
            ret.disp = static_cast<int>(disp_);
            return ret;
        }

        /// Add in the displacement to the LEA operand.
        template <typename T>
        inline operand_base_disp operator-(T disp_) const {
            operand_base_disp ret(this->operator+(disp_));
            ret.disp = -ret.disp;
            return ret;
        }

        /// Implicit conversion from unpacked to packed LEA operand type.
        inline operator typename dynamorio::opnd_t(void) const {
            return dynamorio::opnd_create_base_disp(
                base, index, scale, disp, size);
        }
    };


    /// Defines a generic operand
    struct operand : public dynamorio::opnd_t {
    public:

        inline operand(void) {
            memset(this, 0, sizeof *this);
        }

        inline operand(const dynamorio::opnd_t &&that) {
            memcpy(this, &that, sizeof *this);
        }

        inline operand(const dynamorio::opnd_t &that) {
            memcpy(this, &that, sizeof *this);
        }

        operand(typename dynamorio::reg_id_t reg_) ;

        inline operand &operator=(dynamorio::opnd_t that) {
        	memcpy(this, &that, sizeof *this);
        	return *this;
        }

        /// De-referencing creates a new operand type
        operand_base_disp operator*(void) const ;

        /// Accessing some byte offset from the operand (assuming it points to
        /// some memory)
        operand_base_disp operator[](int num_bytes) const ;

        operand_base_disp operator+(operand index) const ;

        operand_base_disp operator+(operand_base_disp lea) const ;

        /// Construct an LEA operand with an index and scale. This is to
        /// respect the order of operations. Note: valid values of scale are
        /// 1, 2, 4, and 8.
        template <typename I>
        operand_base_disp operator*(I scale) const {
            static_assert(std::is_integral<I>::value,
                "Scale must have an integral type.");

            operand_base_disp ret;
            ret.base = dynamorio::DR_REG_NULL;
            ret.index = value.reg;
            ret.scale = static_cast<int>(scale);
            ret.disp = 0;
            return ret;
        }

        /// Construct an LEA operand with a base and displacement.
        template <typename I>
        operand_base_disp operator+(I disp) const {
            static_assert(std::is_integral<I>::value,
                "Displacement must have an integral type.");

            operand_base_disp ret;
            ret.base = dynamorio::DR_REG_NULL;
            ret.index = value.reg;
            ret.scale = 1;
            ret.disp = static_cast<int>(disp);
            return ret;
        }

        inline operator uint64_t(void) const {
            return value.immed_int;
        }

        inline operator app_pc(void) const {
            return value.pc;
        }

        inline operator typename dynamorio::reg_id_t(void) const {
            return value.reg;
        }
    };


    /// Kind of an operand.
    enum operand_kind {
        SOURCE_OPERAND          = (1 << 0),
        DEST_OPERAND            = (1 << 1),
        SOURCE_DEST_OPERAND     = SOURCE_OPERAND | DEST_OPERAND,
        UNKNOWN
    };


    /// Represents a reference to an operand in an instruction. Useful for
    struct operand_ref {
    private:

        friend struct instruction;

        dynamorio::instr_t *instr;
        operand *op;
        mutable operand *op2;

    public:

        mutable operand_kind kind;

    private:

        operand_ref(
            dynamorio::instr_t *instr_,
            dynamorio::opnd_t *op_,
            operand_kind kind_
        ) ;

    public:

        inline operand_ref(void) 
            : instr(nullptr)
            , op(nullptr)
            , op2(nullptr)
            , kind(UNKNOWN)
        { }

        /// Const accesses of a field of the op are seen as rvalues and need
        /// not invalidate the bits of the instruction.
        inline const operand *operator->(void) const {
            return op;
        }

        inline operand operator*(void) const {
            return *op;
        }

        /// Assume that a non-const access of a field of the op will be used
        /// as an lvalue in an assignment; invalidate the raw bits.
        operand *operator->(void) ;


        /// Copy another reference.
        operand_ref &operator=(const operand_ref &that) ;


        /// Assign an operand to this operand ref; this will update the operand
        /// referenced by this ref in place, and will invalidate the raw bits
        /// of the instruction.
        void replace_with(operand that) ;
        void replace_with(operand_base_disp that) ;


        inline bool is_valid(void) const {
            return nullptr != instr;
        }

        /// Augment this `operand_ref` to a `SOURCE_DEST_OPERAND` if possible
        /// by combining this operand with another.
        void combine(const operand_ref &that) const ;


        /// Returns true iff two `operand_refs` can be combined.
        bool can_combine(const operand_ref &that) const ;
    };


    /// Constraints on how to replace an instruction.
    enum replace_constraint {
        REPLACE_WITH_OLD_META_DATA,
        REPLACE_WITH_NEW_META_DATA,
        REPLACE_WITH_COMBINED_META_DATA
    };


    /// Defines a decoded x86 instruction type. This wraps around DynamoRIO's
    /// Level-3 decoding on x86 instructions.
    struct instruction {
    public:

        template <typename> friend struct list_meta;

        static typename dynamorio::dcontext_t *DCONTEXT;

        enum instruction_flag {
            /// Mark an instruction as already mangled. By default, all
            /// instructions are subject to mangling.
            DONT_MANGLE             = (1 << 0),

            /// Begin a delay region. This property is potentially removed and
            /// back-propagated by the instruction mangler.
            DELAY_BEGIN             = (1 << 1),

            /// End a delay region. This property is potentially removed and
            /// forward-propagated by the instruction mangler.
            DELAY_END               = (1 << 2),

            /// Set iff a label is targeted by a CTI within the current basic
            /// block.
            TARGETED_BY_CTI         = (1 << 3),

            /// Mark this instruction as hot-patchable.
            HOT_PATCHABLE           = (1 << 4),

            /// This is the fall-through jump of a conditional CTI.
            COND_CTI_FALL_THROUGH   = (1 << 5),

            /// Is this a native instruction? That is, did we decode it?
            NATIVE_INSTRUCTION      = (1 << 6)
        };

        typename dynamorio::instr_t *instr;


        /// Constructor
        inline instruction(void) 
            : instr(nullptr)
        { }


        /// Construct an instruction by an instr_t pointer.
        inline instruction(dynamorio::instr_t *instr_) 
            : instr(instr_)
        { }


        /// Replace one instruction with another.
        void replace_with(
            instruction,
            replace_constraint constraint=REPLACE_WITH_COMBINED_META_DATA
        ) ;


        /// Return whether or not this instruction is valid.
        inline bool is_valid(void) const {
            return nullptr != instr;
        }


        inline instruction prev(void) {
            return instruction(instr->prev);
        }


        inline instruction next(void) {
            return instruction(instr->next);
        }

        instruction clone(void) ;


        inline operator bool(void) const {
            return nullptr != instr;
        }

        inline bool operator!(void) const {
            return nullptr == instr;
        }


        inline bool operator==(const instruction &that) const {
            return instr == that.instr;
        }


        inline bool operator!=(const instruction &that) const {
            return instr != that.instr;
        }


        /// Return the code cache policy to be used for the target of this CTI.
        inline instrumentation_policy policy(void) const {
            return instrumentation_policy::decode(instr->granary_policy);
        }


        /// Set the code cache policy to be used for the target of this CTI.
        inline void set_policy(instrumentation_policy policy_) {
            instr->granary_policy = policy_.encode();
        }


        /// Return the opcode of the instruction.
        inline unsigned op_code(void) const {
            return instr->opcode;
        }


        /// Return the number of source operands in this instruction.
        inline unsigned num_sources(void) const {
            return instr->num_srcs;
        }


        /// Return the number of destination operands in this instruction.
        inline unsigned num_destinations(void) const {
            return instr->num_dsts;
        }


        /// Return true iff this instruction is a control-transfer
        /// instruction.
        inline bool is_cti(void) {
            return dynamorio::instr_is_cti(instr);
        }


        /// Return true iff this instruction is a CALL instruction.
        inline bool is_call(void) {
        	return dynamorio::instr_is_call(instr);
        }


        /// Return true iff this instruction is a RET instruction.
        inline bool is_return(void) {
            return dynamorio::instr_is_return(instr);
        }


        /// Return true iff this instruction is a CALL instruction.
        inline bool is_jump(void) {
            return dynamorio::instr_is_jmp(instr);
        }


        /// Return true iff this instruction is a CALL instruction.
        inline bool is_unconditional_cti(void) {
            return is_cti() && !dynamorio::instr_is_cbr(instr);
        }


        /// Return true iff this instruction is a CALL instruction.
		inline bool is_direct_call(void) {
			return dynamorio::instr_is_call_direct(instr);
		}


		/// Return true iff this instruction is a CALL instruction.
		inline bool is_indirect_call(void) {
			return dynamorio::instr_is_call_indirect(instr);
		}


		/// Return true iff this instruction is atomic.
		inline bool is_atomic(void) {
		    return instr->prefixes & PREFIX_LOCK;
		}


        /// Invalidate the raw bits of this instruction.
        inline void invalidate_raw_bits(void) {
            dynamorio::instr_set_raw_bits_valid(instr, false);
            instr->flags &= ~dynamorio::INSTR_RAW_BITS_ALLOCATED;
            instr->flags &= ~dynamorio::INSTR_RAW_BITS_VALID;
            instr->bytes = nullptr;
        }


		/// If this instruction is a CTI, then return the operand
		/// representing the destination of the CTI.
		inline operand cti_target(void) {
		    return dynamorio::instr_get_target(instr);
		}


		/// If this instruction is a CTI, then set the target of the instruction.
        inline void set_cti_target(operand target) {
            invalidate_raw_bits();
            return dynamorio::instr_set_target(instr, target);
        }


        /// Copy the operands of another instruction.
        inline void copy_raw_operands(instruction in) {
            instr->num_dsts = in.instr->num_dsts;
            instr->num_srcs = in.instr->num_srcs;
            instr->u.o.src0 = in.instr->u.o.src0;
            instr->u.o.srcs = in.instr->u.o.srcs;
            instr->u.o.dsts = in.instr->u.o.dsts;
        }


        /// Return the original code program counter from the instruction (if
        /// it exists).
        inline app_pc pc(void) const {
            return instr->translation;
        }


        /// Returns the raw bytes or the PC for the instruction.
        inline app_pc pc_or_raw_bytes(void) const {
            if(instr->translation) {
                return instr->translation;
            }
            return instr->bytes;
        }


        /// Set the program counter of the instruction.
        inline void set_pc(app_pc pc_) {
            invalidate_raw_bits();
            instr->translation = pc_;
        }


        /// Remove a specific granary flag.
        inline void remove_flag(instruction_flag flag) {
            instr->granary_flags &= ~flag;
        }

        /// Remove a specific granary flag.
        inline void add_flag(instruction_flag flag) {
            instr->granary_flags |= flag;
        }

        /// Returns true iff this instruction has a specific flag.
        inline bool has_flag(instruction_flag flag) const {
            return 0 != (flag & instr->granary_flags);
        }


#if CONFIG_ENV_KERNEL
        /// Return true iff this instruction begins a delay region.
        inline bool begins_delay_region(void) const {
            return has_flag(DELAY_BEGIN);
        }


        /// Set this instruction to begin a delay region.
        inline void begin_delay_region(void) {
            add_flag(DELAY_BEGIN);
        }


        /// Return true iff this instruction ends a delay region.
        inline bool ends_delay_region(void) const {
            return has_flag(DELAY_END);
        }


        /// Set this instruction to end a delay region.
        inline void end_delay_region(void) {
            add_flag(DELAY_END);
        }
#endif

        /// Return true iff this instruction is mangled.
        inline bool is_mangled(void) const {
            return has_flag(DONT_MANGLE);
        }


        /// Set the state of the instruction to be mangled.
        inline void set_mangled(void) {
            add_flag(DONT_MANGLE);
        }


        /// Check to see if this instruction can be patched at runtime. If so,
        /// then this instruction needs to be aligned nicely.
        inline bool is_patchable(void) const {
            return has_flag(HOT_PATCHABLE);
        }


        /// Set the state of the instruction to be mangled.
        inline void set_patchable(void) {
            add_flag(HOT_PATCHABLE);
        }


        /// Returns the number of bytes needed to represent this instruction when
        /// it is encoded.
        inline unsigned encoded_size(void) {
            return static_cast<unsigned>(
                dynamorio::instr_length(DCONTEXT, instr));
        }


        /// Widen this instruction if its a CTI.
        void widen_if_cti(void) ;


        /// Decodes a raw byte, pointed to by *pc, and updated *pc to be the
        /// following byte. The decoded instruction is returned by value. If
        /// the instruction cannot be decoded, then *pc is set to NULL.
        void decode_update(
            app_pc *pc,
            instruction_decode_constraint=DECODE_WIDEN_CTI
        ) ;
        static instruction decode(
            app_pc *pc,
            instruction_decode_constraint=DECODE_WIDEN_CTI
        ) ;


        /// Encodes an instruction into a sequence of bytes.
        app_pc encode(app_pc pc) ;


        /// Encodes an instruction into a sequence of bytes, but where the staging
        /// ground is not necessarily the instruction's final location.
        app_pc stage_encode(app_pc staged_pc, app_pc final_pc) ;


        /// Slightly evil convenience method for implicitly converting instructions
        /// to pointers to their underlying DR type.
        inline operator typename dynamorio::instr_t *(void) {
        	return instr;
        }


        /// Apply a function to all of the destination operands and then all
        /// of the source operands.
        template <typename OpRef, typename... Args>
        void for_each_operand(
            void (*func)(OpRef, Args&...),
            Args&... args
        ) {

            // make sure that OpRef is one of:
            //  `operand_ref`           `operand_ref &`
            //  `const operand_ref`     `const operand_ref &`
            static_assert(
                std::is_same<
                    operand_ref,
                    typename std::remove_const<
                        typename std::remove_reference<OpRef>::type
                    >::type
                >::value,
                "Callback of `for_each_operand` must take either an "
                "`operand_ref` or `const operand_ref` instance/reference "
                "as its first argument.");

            operand_ref op;

            if(instr->num_dsts) {
                for(int i(0); i < instr->num_dsts; ++i) {
                    op = operand_ref(instr, &(instr->u.o.dsts[i]), DEST_OPERAND);
                    func(op, args...);
                }
            }

            if(instr->num_srcs) {
                op = operand_ref(instr, &(instr->u.o.src0), SOURCE_OPERAND);
                func(op, args...);

                for(int i(0); i < (instr->num_srcs - 1); ++i) {
                    op = operand_ref(instr, &(instr->u.o.srcs[i]), SOURCE_OPERAND);
                    func(op, args...);
                }
            }
        }
    };


    /// Mark this instruction as already mangled so that it is not mangled
    /// again.
    inline instruction mangled(instruction in) {
        in.set_mangled();
        return in;
    }


    /// Mark this instruction as atomic.
    inline instruction atomic(instruction in) {
        in.instr->prefixes |= PREFIX_LOCK;
        return in;
    }


    /// Mark this instruction as hot patchable.
    inline instruction patchable(instruction in) {
        in.set_patchable();
        return in;
    }


    /// Convert a persistent instruction into a label to the persistent
    /// instruction. This label can be embedded into an instruction list, and
    /// then the persistent instruction can later access the location in memory
    /// where label was encoded.
    instruction persistent_label_(persistent_instruction *in_ptr);
    inline instruction persistent_label_(persistent_instruction &in) {
        return persistent_label_(&in);
    }


    /// Different kinds of instruction lists.
    enum instruction_list_kind {

        /// Instructions being translated.
        INSTRUCTION_LIST_TRANSLATED,

        /// Instructions that have been manually created by Granary code and
        /// are treated as "generated code" to get stuff done. An example of
        /// this would be an IBL entry point, interrupt handler, etc.
        INSTRUCTION_LIST_GENCODE,

        /// Instructions that exist to pass control to a wrapper.
        INSTRUCTION_LIST_WRAPPER,

        /// Instructions that belong to a stub.
        INSTRUCTION_LIST_STUB,

        /// Instruction list that is being staged in a temporary location.
        INSTRUCTION_LIST_STAGED
    };


    /// Represents a generic list of T, where the properties of the list are
    /// configured by specializing list_meta<T>.
    struct instruction_list {
    public:

        typedef instruction_list self_type;

    protected:

        dynamorio::instr_t *first_;
        dynamorio::instr_t *last_;
        unsigned length_;
        instruction_list_kind kind_;

    public:

        /// Initialise an empty list.
        inline instruction_list(
            instruction_list_kind kind=INSTRUCTION_LIST_TRANSLATED
        ) 
            : first_(nullptr)
            , last_(nullptr)
            , length_(0U)
            , kind_(kind)
        { }


        /// Disallow copying in order to maintain consistency.
        instruction_list(self_type &) = delete;
        self_type &operator=(const self_type &) = delete;


        /// Returns the number of elements in the list.
        inline unsigned length(void) const {
            return length_;
        }


        /// Returns true if this instruction list is actually part of a stub
        /// (e.g. for recursively instrumenting memory operations in indirect
        /// CTIs at mangle time).
        inline bool is_stub(void) const {
            return INSTRUCTION_LIST_STUB == kind_;
        }


        /// Clear the elements of the list, and release any memory associated
        /// with the elements of the list
        void clear(void) ;


        /// Return the first element in the list.
        inline instruction first(void) const {
            if(!first_) {
                return instruction(nullptr);
            }

            return instruction(first_);
        }

        /// Return the last element in the list.
        inline instruction last(void) const {
            if(!last_) {
                return instruction(nullptr);
            }

            return instruction(last_);
        }

        /// Adds another instruction list to the end of the current one.
        ///
        /// Note: This removed all elements from the argument list.
        void extend(instruction_list &that) ;

        /// Adds an element on to the end of the list.
        instruction append(instruction item_) ;

        /// Adds an element on to the beginning of the list.
        instruction prepend(instruction item_) ;

        /// Insert an element before another object in the list.
        instruction insert_before(instruction after_item_, instruction item_) ;

        /// Remove an element from an instruction list.
        void remove(instruction to_remove) ;

        /// Remove a tail of the list, starting at an instruction.
        void remove_tail_at(instruction at_item_) ;

        /// Insert an element after another object in the list
        instruction insert_after(instruction before_item_, instruction item_) ;

        /// The encoded size of the instruction list.
        unsigned encoded_size(void) ;

        /// Encodes an instruction list into a sequence of bytes. The user of
        /// this API is required to know in advance the exact size of the
        /// instruction list, so that a good allocation discipline is followed.
        app_pc encode(app_pc pc, unsigned exact_size) ;

        /// Performs a staged encoding of an instruction list into a sequence
        /// of bytes.
        ///
        /// Note: This will not do any fancy jump resolution, alignment, etc.
        app_pc stage_encode(app_pc staged_pc, app_pc final_pc) ;

    protected:

        /// Chain an element into the list.
        instruction chain(
            dynamorio::instr_t *before_item,
            dynamorio::instr_t *item,
            dynamorio::instr_t *after_item
        ) ;

    } __attribute__((packed));


    /// Registers
#define MAKE_REG(name, upper_name) extern operand name;
#define MAKE_SEG(name, upper_name)
    namespace reg {
#   include "granary/x86/registers.h"
    }
#undef MAKE_SEG
#undef MAKE_REG


    /// Segment registers.
#define MAKE_REG(name, upper_name)
#define MAKE_SEG(name, upper_name) \
    struct CAT(segment_, name) { \
        inline operand operator()(operand op) { \
            op.seg.segment = dynamorio::DR_ ## upper_name ; \
            return op;\
        } \
        inline operand operator()(operand_base_disp op_) { \
            operand op(op_); \
            op.seg.segment = dynamorio::DR_ ## upper_name ; \
            return op; \
        } \
        inline operand operator[](int offset) { \
            operand op; \
            op.seg.segment = dynamorio::DR_ ## upper_name ; \
            op.seg.disp = dynamorio::DR_ ## upper_name ; \
            op.seg.shift = dynamorio::DR_ ## upper_name ; \
            op.seg.far_pc_seg_selector = dynamorio::DR_ ## upper_name ; \
            if(!offset) { \
                op.value.immed_int = 0; \
                op.value.base_disp.encode_zero_disp = true; \
            } else { \
                op.value.base_disp.disp = offset; \
            } \
            op.size = dynamorio::OPSZ_8; \
            op.kind = dynamorio::BASE_DISP_kind; \
            return op; \
        } \
    }; \
    extern CAT(segment_, name) name;

    namespace seg {
#   include "granary/x86/registers.h"
    }
#undef MAKE_SEG
#undef MAKE_REG
}

#include "granary/gen/instruction.h"


namespace granary {
    /// Create an instruction label to some (almost) arbitrary data. The label
    /// can be used to get the address of the data using LEA.
    template <typename T>
    operand data_label_(T *data) {
        instruction label = label_();
        label.instr->note = unsafe_cast<void *>(data);
        label.instr->translation = unsafe_cast<app_pc>(data);
        switch(sizeof(T)) {
        case 8: return dynamorio::opnd_create_mem_instr(label, 0, dynamorio::OPSZ_8);
        case 4: return dynamorio::opnd_create_mem_instr(label, 0, dynamorio::OPSZ_4);
        case 2: return dynamorio::opnd_create_mem_instr(label, 0, dynamorio::OPSZ_2);
        default: return dynamorio::opnd_create_mem_instr(label, 0, dynamorio::OPSZ_1);
        }
    }
}

#endif /* granary_INSTRUCTION_H_ */
