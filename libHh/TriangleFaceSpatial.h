// -*- C++ -*-  Copyright (c) Microsoft Corporation; see license.txt
#ifndef MESH_PROCESSING_LIBHH_TRIANGLEFACESPATIAL_H_
#define MESH_PROCESSING_LIBHH_TRIANGLEFACESPATIAL_H_

#include "libHh/Facedistance.h"
#include "libHh/GMesh.h"
#include "libHh/GeomOp.h"
#include "libHh/Spatial.h"
#include "libHh/Timer.h"

namespace hh {

struct TriangleFace {
  Vec3<Point> triangle;
  Face face;
};

namespace details {

struct triangleface_approx_distance2 {
  float operator()(const Point& p, Univ id) const {
    const TriangleFace& triangleface = *Conv<const TriangleFace*>::d(id);
    const Vec3<Point>& triangle = triangleface.triangle;
    return square(lb_dist_point_triangle(p, triangle[0], triangle[1], triangle[2]));
  }
};

struct triangleface_distance2 {
  float operator()(const Point& p, Univ id) const {
    const TriangleFace& triangleface = *Conv<const TriangleFace*>::d(id);
    const Vec3<Point>& triangle = triangleface.triangle;
    return dist_point_triangle2(p, triangle[0], triangle[1], triangle[2]);
  }
};

}  // namespace details

// A spatial data structure over a collection of triangles, each associated with a Mesh Face.
class TriangleFaceSpatial
    : public ObjectSpatial<details::triangleface_approx_distance2, details::triangleface_distance2> {
 public:
  explicit TriangleFaceSpatial(CArrayView<TriangleFace> trianglefaces, int gridn) : ObjectSpatial(gridn) {
    HH_STIMER("__trianglefacespatial_build");
    for (const TriangleFace& triangleface : trianglefaces) {
      const Vec3<Point>& triangle = triangleface.triangle;
      // TODO: Speed this up by avoiding use of Polygon.
      Polygon poly{triangle}, opoly = poly;
      const Bbox bbox{triangle};
      const auto func_triangleface_in_bbox = [&](const Bbox<float, 3>& spatial_bbox) -> bool {
        for_int(c, 3) if (bbox[0][c] > spatial_bbox[1][c] || bbox[1][c] < spatial_bbox[0][c]) return false;
        const bool modified = poly.intersect_bbox(spatial_bbox);
        const bool ret = poly.num() > 0;
        if (modified) poly = opoly;
        return ret;
      };
      ObjectSpatial::enter(Conv<const TriangleFace*>::e(&triangleface), triangle[0], func_triangleface_in_bbox);
    }
  }
  // clear() inherited from ObjectSpatial

  bool first_along_segment(const Point& p1, const Point& p2, const TriangleFace*& ret_ptriangleface,
                           Point& ret_pint) const {
    Vector vray = p2 - p1;
    float tmin = BIGFLOAT;
    const auto func_test_triangleface_with_ray = [&](Univ id) -> bool {
      const TriangleFace* ptriangleface = Conv<const TriangleFace*>::d(id);
      const Vec3<Point>& triangle = ptriangleface->triangle;
      const auto pint = intersect_segment(triangle, p1, p2);
      if (!pint) return false;
      const float t = dot(*pint - p1, vray);
      if (t < tmin) {
        tmin = t;
        ret_pint = *pint;
        ret_ptriangleface = ptriangleface;
      }
      return true;
    };
    search_segment(p1, p2, func_test_triangleface_with_ray);
    const bool found_intersection = tmin < BIGFLOAT;
    return found_intersection;
  }
};

}  // namespace hh

#endif  // MESH_PROCESSING_LIBHH_TRIANGLEFACESPATIAL_H_