#include "minimum_degree_heuristic.hpp"

#include <random>
#include <queue>
#include <utility>
#include <vector>

namespace {

using ElimOrder = std::vector<Vertex>;

TD elimination_ordering_to_td(Graph &graph, ElimOrder order) {
  always_assert(graph.num_vertices() == order.size());

  TD td;
  std::vector<size_t> time_processed(graph.num_vertices(),
                                     std::numeric_limits<size_t>::max());
  std::vector<TD::Bag> bag_of_vertex(graph.num_vertices());

  size_t time = 0;
  for (auto itr = order.crbegin(); itr != order.crend(); ++itr, ++time) {
    Vertex v = *itr;

    time_processed[v] = time;

    std::vector<Vertex> neighbors;
    size_t max_time = 0;
    TD::Bag max_bag;
    for (Vertex w : graph.neighbors(v)) {
      if (time_processed[w] < time) {
        neighbors.push_back(w);
        if (time_processed[w] >= max_time) {
          max_time = time_processed[w], max_bag = bag_of_vertex[w];
        }
      }
    }

    if (neighbors.empty()) {
      bag_of_vertex[v] = td.add_leaf(v);
    } else {
      neighbors.push_back(v);
      bag_of_vertex[v] = td.add_child(max_bag, std::move(neighbors));
    }
  }

  
  return td;
}

void triangulate(Graph &graph, Vertex v) {
  const auto &neighbors = graph.neighbors(v);

  for (Vertex w : neighbors)
    graph.remove_arc(w, v);

  if (neighbors.size() < 2)
    return;

  for (size_t i1 = 0; i1 < neighbors.size() - 1; ++i1) {
    Vertex w1 = neighbors[i1];
    for (auto i2 = i1 + 1; i2 < neighbors.size(); ++i2) {
      Vertex w2 = neighbors[i2];
      always_assert(w2 < graph.num_vertices());
      if (!graph.adjacent(w1, w2) && !graph.adjacent(w2, w1))
        graph.add_arc(w1, w2), graph.add_arc(w2, w1);
    }
  }
}

float noise() {
  static uint32_t x=123456789, y=362436069, z=521288629;
  x ^= x << 16; x ^= x >> 5; x ^= x << 1;
  uint32_t t = x;
  x = y;
  y = z;
  z = t ^ x ^ y;
  return (z & 0xFFFF) / ((float) 0xFFF0);
}
}

TD minimum_degree_heuristic(Graph graph) {
  ElimOrder order;
  order.reserve(graph.num_vertices());

  std::vector<bool> seen(graph.num_vertices(), false);
  std::vector<size_t> prev_deg(graph.num_vertices());
  using QElement = std::pair<float, Vertex>;
  std::priority_queue<QElement, std::vector<QElement>, std::greater<QElement>> pq;

  for (Vertex v : graph.vertices()) {
    prev_deg[v] = graph.degree(v);
    pq.emplace(prev_deg[v] + noise(), v);
  }

  while (pq.size() > 0) {
    float k = pq.top().first;
    Vertex v = pq.top().second;
    pq.pop();

    if (seen[v])
      continue;

    if ((size_t) k != graph.degree(v)) {
      prev_deg[v] = graph.degree(v);
      pq.emplace(prev_deg[v] + noise(), v);
      continue;
    }

    order.push_back(v);
    seen[v] = true;

    triangulate(graph, v);

    for (Vertex w : graph.neighbors(v)) {
      if (graph.degree(w) < prev_deg[w]) {
        prev_deg[w] = graph.degree(w);
        pq.emplace(prev_deg[w] + noise(), w);
      }
    }
  }

  for (Vertex v : graph.vertices())
    for (Vertex w : graph.neighbors(v))
      graph.add_arc(w, v);

  return elimination_ordering_to_td(graph, order);
}
