#include <cstdlib>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

// ----------------------------------------------------------------

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

struct Map
{
  typedef std::unordered_set<Coords, CoordsHasher> BlockSet;

  unsigned width, height;

  BlockSet blocks;
};

Map
read_map (std::istream &in)
{
  Map map;
  in >> map.width;
  in >> map.height;

  unsigned x, y;
  while (in >> x)
    {
      in >> y;
      map.blocks.insert (Coords (x, y));
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

    if (end_x >= map.width || end_y >= map.height)
      return 0;
  
    Coords end_coords (end_x, end_y);

    // See if we've already computed this location, and if so just
    // return the previous result.
    //
    auto prev_result = lookup_cache.find (end_coords);
    if (prev_result != lookup_cache.end ())
      return prev_result->second;

    if (map.blocks.count (end_coords))
      return 0;

    unsigned v_count = (end_y == 0) ? 0 : num_routes (end_x, end_y - 1);
    unsigned h_count = (end_x == 0) ? 0 : num_routes (end_x - 1, end_y);

    unsigned count = v_count + h_count;

    if (v_count && h_count)
      {
	count -= num_routes (end_x - 1, end_y - 1);
	count += 1;
      }

    lookup_cache.emplace (end_coords, count);

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

int main (int argc, const char *const *argv)
{
  if (argc != 5)
    {
      std::cerr << "Usage: " << argv[0] << " START_X START_Y END_X END_Y\n";
      exit (0);
    }
  Map map = read_map (std::cin);

  std::cout << "Map size: " << map.width << ", " << map.height << '\n';
  std::cout << "Blocks:" << '\n';
  for (auto &block : map.blocks)
    std::cout << "  " << block.x << ", " << block.y << '\n';

  unsigned start_x = atoi (argv[1]), start_y = atoi (argv[2]);
  unsigned end_x = atoi (argv[3]), end_y = atoi (argv[4]);
  unsigned count = num_routes (map, start_x, start_y, end_x, end_y);

  std::cout << "Number of routes from (" << start_x << ", " << start_y << ")"
	    << " to (" << end_x << ", " << end_y << "): " << count << '\n';

  return 0;
}
