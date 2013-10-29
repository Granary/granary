"""Used to decide what bits of a kernel address are useful for hashing.

Note: This focuses on functions only, as we make the assumption that
      all indirect CTIs (excluding RETs) target functions.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

import os

BAD_BIT = 0.00001

def find_dist(addresses):

  DIST_ONE = list([1.0] * 64)[:]
  DIST_ZERO = list([1.0] * 64)[:]
  DIST = list([BAD_BIT] * 64)[:]
  BOUND_MIN = 0.3
  BOUND_MAX = 0.7

  for addr in addresses:
    bits = list(enumerate(reversed(bin(addr)[2:])))

    if 64 > len(bits):
      continue

    for i, bit in bits:
      if "1" == bit:
        DIST_ONE[i] += 1
      else:
        DIST_ZERO[i] += 1

  for i in range(64):
    bound = DIST_ZERO[i] / (DIST_ONE[i] + DIST_ZERO[i])
    if BOUND_MIN <= bound <= BOUND_MAX:
      DIST[i] = bound

  return DIST

def xchg_16(mask):
  return "XCHG L8, H8", mask[8:16] + mask[0:8] + mask[16:]

def bswap_32(mask):
  return "BSWAP 32", mask[24:32] + mask[16:24] + mask[8:16] + mask[0:8] + mask[32:]

def bswap_64(mask):
  return "BSWAP 64", mask[56:64] + mask[48:56] + mask[40:48] + mask[32:40] + mask[24:32] + mask[16:24] + mask[8:16] + mask[0:8]

def mul_2(mask):
  return "LEA multiply 2", [0] + mask[:-1]

def mul_4(mask):
  return "LEA multiply 4", [0, 0] + mask[:-2]

def mul_8(mask):
  return "LEA multiply 8", [0, 0, 0] + mask[:-3]

OPS = [
  xchg_16, bswap_32, mul_2, mul_4, mul_8, bswap_64
]


def score_mask8(mask, scale):
  total = sum(mask)
  penalty = max(mask) / 2
  for b in mask:
    if not b or b == BAD_BIT: # Penalize bad bits.
      total -= penalty
  return total * scale


COMPRESSIONS = {
  ("LEA multiply 2", "LEA multiply 4"): "LEA multiply 8",
  ("LEA multiply 4", "LEA multiply 2"): "LEA multiply 8",

  ("LEA multiply 2", "LEA multiply 2"): "LEA multiply 4",
}

DELETIONS = set([
  ("BSWAP 32", "BSWAP 32"),
  ("BSWAP 64", "BSWAP 64"),
  ("XCHG L8, H8", "XCHG L8, H8")
])

REVERSIBLE = set([
  ("LEA multiply 2", "LEA multiply 8"),
  ("LEA multiply 4", "LEA multiply 8"),
  ("LEA multiply 8", "LEA multiply 2"),
  ("LEA multiply 8", "LEA multiply 4"),
])


def can_compress(hist, searches):
  changed = True
  i = 0

  if len(hist) < 2:
    return False

  key = tuple(hist[:-2])
  if key in COMPRESSIONS or key in DELETIONS:
    return True

  if key in REVERSIBLE:
    new_hist = tuple(hist[:-2] + list(reversed(key)))
    if new_hist in searches:
      return True

  return False


def score_mask(mask):
  num_zeros = 0
  num_leading_zeros = 0
  num_bad_bits = 0
  num_good_bits = 0
  good_bit_sum = 0.0
  good_bit_sway = 0.0
  done_leading_zeros = False

  for i, b in enumerate(mask[:16]):
    if not done_leading_zeros and not b:
      num_leading_zeros += 1
    if not b:
      num_zeros += 1
    else:
      done_leading_zeros = True
      if b == BAD_BIT:
        num_bad_bits += 1
      else:
        num_good_bits += 1
        good_bit_sum += b
        good_bit_sway += abs(b - 0.5)

  leading_zeroes_best_score = 6
  good_bit_sum_best_score = 0.5 * (16 - leading_zeroes_best_score)
  best_score = leading_zeroes_best_score + good_bit_sum_best_score

  # Find our score
  score = 0.0
  if leading_zeroes_best_score == num_leading_zeros:
    score += leading_zeroes_best_score

  score += good_bit_sum
  score -= good_bit_sway

  # Penalize the presence of "bad bits"
  score -= num_bad_bits * 0.5

  # Penalize the presence of zeroes
  score -= num_zeros - num_leading_zeros

  return score / best_score


def search(mask, prev_ops, searches, depth):
  if depth:

    # We've explored this path already through a shorter means.
    if can_compress(prev_ops, searches):
      return

    old_len = len(searches)
    searches.add((score_mask(mask), tuple(mask), tuple(prev_ops)))
    if depth > 7 or old_len == len(searches):
      return

  for op in OPS:
    op_desc, op_mask = op(mask)
    op_hist = prev_ops + [op_desc]    
    search(op_mask, op_hist, searches, depth + 1)
    

if __name__ == "__main__":
  kernel_addresses = set()
  predicted_addresses = set()
  predicted_addresses_str = set([
    'fffffff81154e30', 'ffffffff811c0890', 'ffffffff810ae7b0', 'ffffffff8153890',
    'ffffffff815ef280', 'ffffffff81159a60', 'ffffffff814e0930', 'ffffffff811c3020',
    'ffffffff81165630', 'ffffffff81153f20', 'ffffffff815ef030', 'ffffffffa00fa570',
    'ffffffff811717b0', 'ffffffff81060b20', 'ffffffff814e0920', 'fffffff81153890',
    'ffffffff814e0590', 'ffffffff81168d50', 'ffffffff81060890', 'ffffffff81152e90',
    'ffffffff811598f0', 'ffffffff810020d0', 'ffffffff8137a7c0', 'ffffffff8117a210',
    'fffffff81154d90', 'ffffffff81060a90', 'ffffffff81153f0', 'ffffffff81074260',
    'fffffff811598f0', 'ffffffff81155520', 'ffffffff814e090', 'ffffffff81061d70',
    'ffffffff814e0850', 'ffffffff81167070', 'ffffffff81050bd0', 'ffffffff81060b80',
    'ffffffff814e0980', 'fffffff814e0980', 'ffffffff814e020', 'ffffffff81179ac0',
    'ffffffff81159920', 'fffffff81167070', 'ffffffff810741a0', 'fffffff810079e0',
    'ffffffff8115380', 'ffffffff81061a40', 'ffffffff81050270', 'ffffffff81056400',
    'ffffffff810079e0', 'fffffff81127200', 'ffffffff84e0980', 'ffffffff81166c10',
    'ffffffff81060bb0', 'ffffffff811538f0', 'ffffffff8116cff0', 'ffffffff81061c80',
    'ffffffff81154d90', 'ffffffff814e0640', 'ffffffff81179850', 'ffffffff81056280',
    'ffffffff81180fa0', 'fffffff81155520', 'ffffffff81159900', 'ffffffff81168e50',
    'ffffffff81060580', 'ffffffff810606f0', 'ffffffff814e0ec0', 'ffffffff814e0950',
    'fffffff81153f20', 'ffffffff81060b50', 'ffffffff81125460', 'ffffffff81153890',
    'ffffffff81194310', 'ffffffff8127200', 'ffffffff81060760', 'ffffffff811b7a70',
    'ffffffff8104c9a0', 'ffffffff811538b0', 'ffffffff81059b80', 'ffffffff81060d00',
    'ffffffff8115dbc0', 'ffffffff81127200', 'ffffffff81060250', 'ffffffff81154e30',
    'ffffffff81061140', 'fffffff814e0950'
  ])

  with open("kernel.syms", "r") as lines_:
    for line in lines_:
      line = line.strip(" \r\n\t")
      line = line.replace("\t", " ")
      parts = line.split(" ")
      sym = parts[2]
      
      addr = int(parts[0], base=16)
      kernel_addresses.add(addr)

  for addr in predicted_addresses_str:
    addr = int(addr, base=16)
    predicted_addresses.add(addr)

  kernel_dist = find_dist(kernel_addresses)
  predict_dist = find_dist(predicted_addresses)

  # Show the combined "good bits" from both kernel addresses, and recorded
  # mis-predicted addresses from a runtime trial.
  mask = [0] * 64
  for i, (k, p) in enumerate(zip(kernel_dist, predict_dist)):
    if abs(k - 0.5) > abs(p - 0.5):
      mask[i] = k
    else:
      mask[i] = p

  print mask
  
  # Search for a good hash function
  searches = set()
  search(mask, [], searches, 0)

  # Sort the hash functions by their output value.
  searches = list(searches)
  searches.sort(key=lambda r: (r[0], -len(r[2])), reverse=True)

  # Display the top 5 hash functions.
  for result in searches[:5]:
    value, mask, steps = result
    print value
    print "\t",
    print "\n\t".join(steps)
    for b in mask[:16]:
      if not b:
        print "\tZ", 
      elif b == BAD_BIT:
        print "\tB", 
      else:
        print "\t%d" % abs(int((b - 0.5) * 100)), 
    print
    print