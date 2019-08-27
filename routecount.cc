#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

// ----------------------------------------------------------------
// Grid coordinate type

struct Coords
{
  Coords (unsigned _x, unsigned _y) : x (_x), y (_y) { }
  Coords (const Coords &from) : x (from.x), y (from.y) { }

  bool operator== (const Coords &other) const
  {
    return x == other.x && y == other.y;
  }

  unsigned x, y;

};

struct CoordsHasher
{
  std::size_t operator() (const Coords &coords) const
  {
    return coords.x ^ coords.y;
  }
};

// ----------------------------------------------------------------
// Map.  Maps are rectangular, with a grid of intersections connected
// by horizontal and vertical streets.

// Bits in a bitmap representing the possible street directions from a node.
//
enum StreetType { STREET_HORIZ = 1, STREET_VERT = 2 };

// A set of street types, one bit for each type.
//
typedef unsigned StreetTypeSet;

// Map.  Maps are rectangular, with a grid of intersections connected
// by horizontal and vertical streets.  They are represented by a size
// and a list of blocked-off streets; any street not recorded as
// blocked off is assumed to be open.
//
class Map
{
public:
  Map (unsigned _width, unsigned _height)
    : width (_width), height (_height)
  { }

  // Remember that the incoming streets in STREETS are blocked for the
  // intersection at (X, Y).
  //
  void add_blocks (unsigned x, unsigned y, StreetTypeSet streets)
  {
    Coords coords (x, y);

    // See if there's an existing entry for COORDS.
    auto block_it = blocks.find (coords);
    if (block_it == blocks.end ())
      // No existing entry. Just add a new one.
      blocks.emplace (coords, streets);
    else
      // Update the existing entry, adding STREETS to any existing blocks.
      block_it->second |= streets;
  }

  // Return which incoming streets in STREETS are blocked for the
  // intersection at (X, Y).  Streets which would come from outside
  // the map are always considered blocked.
  //
  StreetTypeSet get_blocks (unsigned x, unsigned y) const
  {
    Coords coords (x, y);

    // Find blocked streets entry.
    //
    auto block_it = blocks.find (coords);

    // If there is no entry for (X, Y), use the empty set.
    //
    StreetTypeSet streets
      = (block_it == blocks.end ()) ? 0 : block_it->second;

    // If X or Y are at the (left or top) edge of the map, also
    // consider the appropriate street blocked.
    //
    if (x == 0)
      streets |= STREET_HORIZ;
    if (y == 0)
      streets |= STREET_VERT;

    return streets;
  }

  // Accessors for map size.
  //
  unsigned get_width () const { return width; }
  unsigned get_height () const { return height; }

private:

  // Streets blocked in this map.  For each intersection, the set of
  // blocked streets _leading_ to it are recorded; that is, if
  // STREET_HORIZ is in the set, then the street to the _left_ of the
  // intersections is blocked, and if STREET_VERT is in the set, then
  // the street _above_ the intersection is blocked.
  //
  typedef std::unordered_map<Coords, StreetTypeSet, CoordsHasher> BlockSet;

  // Size of the map.  Valid x coordinates in the map are in the range
  // [0, WIDTH), and valid y coordinates in the range [0, HEIGHT).
  //
  unsigned width, height;

  BlockSet blocks;
};

// Return a new Map read in from the stream IN.
// The format is:
//   WIDTH HEIGHT
//   ISEC_X ISEC_Y INCOMING_BLOCKS
//   ...
// where WIDTH and HEIGHT are the integer width and height of the
// map, ISEC_X and ISEC_Y are the coordinates of an intersection, and
// INCOMING_BLOCKS is an integer desc which streets
// into that interseciton are blocked:  1 being the horizontal street
// to the left, 2 being the vertical street above, and 3 being both.
//
Map
read_map (std::istream &in)
{
  unsigned width, height;
  in >> width;
  in >> height;

  Map map (width, height);

  unsigned x, y;
  while (in >> x)
    {
      StreetTypeSet streets;

      in >> y;
      in >> streets;
      map.add_blocks (x, y, streets);
    }

  return map;
}

// ----------------------------------------------------------------

struct SearchState
{
  SearchState (const Map &_map, unsigned _start_x, unsigned _start_y)
    : map (_map), start_x (_start_x), start_y (_start_y)
  { }

  unsigned num_routes (unsigned end_x, unsigned end_y)
  {
    if (end_x < start_x || end_y < start_y)
      return 0;

    if (end_x == start_x && end_y == start_y)
      return 1;

    if (end_x >= map.get_width() || end_y >= map.get_height())
      return 0;
  
    Coords end_coords (end_x, end_y);

    // See if we've already computed the routes to this intersection.
    //
    // If we have, then that represents a shared segment of routes for
    // this computation (that is, the routes to that point have
    // already been counted for some other path).
    //
    // In this case, we just return 0 or 1: If this intersection was
    // unreachable, of course we return 0, otherwise we don't want
    // to re-count the routes in the shared segment, but we _do_ want
    // to reflect the new route from this intersection to our caller.
    // The appropriate value to return is recorded in lookup_cache by
    // the initial probe which visited this intersection.
    //
    auto prev_result = lookup_cache.find (end_coords);
    if (prev_result != lookup_cache.end ())
      return prev_result->second;

    // Find which incoming streets into this intersection are blocked.
    //
    StreetTypeSet incoming_blocks = map.get_blocks (end_x, end_y);
    
    unsigned count = 0;
    if (! (incoming_blocks & STREET_HORIZ))
      count += num_routes (end_x - 1, end_y);
    if (! (incoming_blocks & STREET_VERT))
      count += num_routes (end_x, end_y - 1);

    // Record that we've already looked here, and if so, the number of
    // routes to any new caller.  If this place is unreachable, that's
    // zero, otherwise one.
    //
    lookup_cache.emplace (end_coords, count == 0 ? 0 : 1);

    return count;
  }

  const Map &map;

  unsigned start_x, start_y;

  std::unordered_map<Coords, unsigned, CoordsHasher> lookup_cache;
};

unsigned
num_routes (const Map &map,
	    unsigned start_x, unsigned start_y,
	    unsigned end_x, unsigned end_y)
{
  SearchState search_state (map, start_x, start_y);
  return search_state.num_routes (end_x, end_y);
}

// ----------------------------------------------------------------

// Draw an ASCII picture of MAP, with route counts for each
// intersection starting from (START_X, START_Y).
//
void
draw_map_with_counts (const Map &map, unsigned start_x, unsigned start_y)
{
  unsigned width = map.get_width();
  unsigned height = map.get_height();

  for (unsigned y = 0; y < height; y++)
    {
      if (y > 0)
	{
	  for (unsigned vline = 0; vline < 3; vline++)
	    {
	      for (unsigned x = 0; x < width; x++)
		{
		  if (x > 0)
		    std::cout << "   ";
		  if (vline == 1 && (map.get_blocks (x, y) & STREET_VERT))
		    std::cout << "  X";
		  else
		    std::cout << "  |";
		}
	      std::cout << '\n';
	    }
	}

      for (unsigned x = 0; x < width; x++)
	{
	  if (x > 0)
	    {
	      if (map.get_blocks (x, y) & STREET_HORIZ)
		std::cout << "-X-";
	      else
		std::cout << "---";
	    }

	  if (x == 0)
	    std::cout << std::setfill (' ') << std::setw (3);
	  else
	    std::cout << std::setfill ('-') << std::setw (3);

	  if (x == start_x && y == start_y)
	    std::cout << "@";
	  else if (x >= start_x && y >= start_y)
	    std::cout << num_routes (map, start_x, start_y, x, y);
	  else
	    std::cout << "+";
	}
      std::cout << '\n';
    }
}

// ----------------------------------------------------------------

int main (int argc, const char *const *argv)
{
  if (argc != 5 && argc != 3)
    {
      std::cerr << "Usage: " << argv[0] << " START_X START_Y [END_X END_Y]\n";
      exit (0);
    }
  Map map = read_map (std::cin);

  std::cout << "Map size: " << map.get_width() << ", " << map.get_height() << '\n';

  unsigned start_x = atoi (argv[1]), start_y = atoi (argv[2]);

  if (argc == 5)
    {
      unsigned end_x = atoi (argv[3]), end_y = atoi (argv[4]);
      unsigned count = num_routes (map, start_x, start_y, end_x, end_y);

      std::cout << "Number of routes from (" << start_x << ", " << start_y << ")"
		<< " to (" << end_x << ", " << end_y << "): " << count << '\n';
    }
  else
    {
      draw_map_with_counts (map, start_x, start_y);
    }

  return 0;
}
