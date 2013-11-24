
import sys


class BasicBlock(object):

  __slots__ = (
    'cache_start', 'num_bytes', 'cache_end',
    'allocator', 'trace_len', 'native_start', 'count',
    'module_offset', 'module_name',
    'x', 'y', 'edge_strings',
  )

  def __init__(self, line):
    parts = line[len("BB("):-len(")")].split(",")
    self.cache_start = int(parts[0], 16)
    self.num_bytes = int(parts[1])
    self.cache_end = self.cache_start + self.num_bytes
    self.allocator = int(parts[2], 16)
    self.trace_len = int(parts[3])
    self.native_start = int(parts[4], 16)
    self.edge_strings = []

    self.x = 0
    self.y = 0

    self.count = 0
    if 8 == len(parts) or 6 == len(parts):
      self.count = int(parts[5])

    if 7 <= len(parts):
      self.module_offset = int(parts[-2], 16)
      self.module_name = parts[-1]


BBS = []
BB_ALIGN = 64
PAGE_SIZE = 4096
SLAB_SIZE = PAGE_SIZE
EDGE_STRINGS = {}
MAX_SLABS_X_AXIS = 16
BYTE_RATIO = 8
MAX_SLAB_WIDTH_PX = 32

if __name__ == "__main__":
  with open(sys.argv[1], "r") as lines:
    last_bb = None
    for line in lines:
      if line.startswith("BB"):
        last_bb = BasicBlock(line.strip(" \r\n\t"))
        BBS.append(last_bb)
      elif line.strip():
        assert last_bb
        last_bb.edge_strings.append(line.strip(" \r\n\t"))

  BBS.sort(key=lambda bb: bb.cache_start)

  min_cache_pc = BBS[0].cache_start
  max_cache_pc = BBS[-1].cache_end
  
  aligned_max_cache_pc = ((max_cache_pc + (PAGE_SIZE - 1)) / PAGE_SIZE) * PAGE_SIZE
  aligned_min_cache_pc = (min_cache_pc / PAGE_SIZE) * PAGE_SIZE

  print "Number of basic blocks:", len(BBS)
  print "Code cache size:", (aligned_max_cache_pc / 2.0**20)

  min_slab_id = min_cache_pc / SLAB_SIZE
  max_slab_id = max_cache_pc / SLAB_SIZE

  # Create a partitioning of basic blocks into their respective slabs.
  num_slabs = max_slab_id - min_slab_id + 1
  slabs = []
  for _ in range(num_slabs):
    slabs.append([])

  # Assign basic into slabs.
  for bb in BBS:
    slab_id = bb.cache_start / SLAB_SIZE
    slabs[slab_id - min_slab_id].append(bb)

  # Remove empty slabs
  #slabs = filter(lambda slab: len(slab) > 0, slabs)
  #print "Num unused slabs:", num_slabs - len(slabs)
  #num_slabs = len(slabs)

  max_height = max(len(slab) for slab in slabs)
  max_width = min(
    MAX_SLAB_WIDTH_PX,
    max((bb.num_bytes + (BB_ALIGN - 1)) / BYTE_RATIO for bb in BBS))

  num_slab_rows = (num_slabs + MAX_SLABS_X_AXIS - 1) / MAX_SLABS_X_AXIS

  WIDTH_PX = max_width * MAX_SLABS_X_AXIS
  HEIGHT_PX = max_height * num_slab_rows

  import Image, ImageDraw, ImageFont
  font = ImageFont.load("/home/pag/Code/pilfonts/courB08.pil")
  img = Image.new('RGB', (WIDTH_PX, HEIGHT_PX), "black")

  allocators = set(bb.allocator for bb in BBS)
  allocators = list(allocators)
  allocators.sort()
  allocator_nums = dict(zip(allocators, range(len(allocators))))
  allocators = set(allocators)

  # Get a color palette: http://stackoverflow.com/a/876872/247591
  import colorsys
  N = len(allocators)
  print "Number of used allocators:", N
  HSV_tuples = [(x*1.0/N, (N-x)*1.0/N, 0.5) for x in range(N)]
  RGB_tuples = map(lambda x: colorsys.hsv_to_rgb(*x), HSV_tuples)
  
  colors = {}
  for allocator, color in zip(allocators, RGB_tuples):
    color = (int(color[0] * 255), int(color[1] * 255), int(color[2] * 255))
    colors[allocator] = color

  pixels = img.load() # create the pixel map
  draw = ImageDraw.Draw(img)

  for i, slab in enumerate(slabs):
    start_x = (i % MAX_SLABS_X_AXIS) * max_width
    y_mult = i / MAX_SLABS_X_AXIS

    for y, bb in enumerate(slab):
      start_y = y + (max_height * y_mult)

      bb.x = start_x
      bb.y = start_y
      width = min(
        MAX_SLAB_WIDTH_PX,
        max(1, (bb.num_bytes + (BYTE_RATIO - 1)) / BYTE_RATIO))
      for x_off in range(width):
        color = colors[bb.allocator]
        if not bb.count:
          color = (0xff, 0, 0)

        pixels[start_x + x_off, start_y] = color

    if slab:
      some_allocator = slab[0].allocator
      draw.text(
        (start_x + MAX_SLAB_WIDTH_PX / 3, max_height * (1 + y_mult) - 20),
        str(allocator_nums[some_allocator]),
        font=font)



  #for i in range(img.size[0]):    # for every pixel:
  #  for j in range(img.size[1]):
  #    pixels[i,j] = (i, j, 100) # set the colour accordingly
  
  img.show()

  #print max_cache_pc, min_cache_pc
