// A nanobind wrapper of client.h

#include "client_ext.h"

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>

#include "frame.h"
#include "third_party/status/exceptions.h"

namespace nb = nanobind;

std::unique_ptr<Desktop> Desktop::create(
    int32_t width, int32_t height, const std::vector<std::string>& command) {
  ASSIGN_OR_RAISE(auto backend,
                  WaylandBackend::start_server(
                      /*port_offset=*/5900, width, height, command));
  auto desktop = std::unique_ptr<Desktop>(new Desktop());
  desktop->backend_ = std::move(backend);
  RAISE_IF_ERROR(desktop->connect_impl(desktop->backend_->port()));
  return desktop;
}

NB_MODULE(bounce_desktop, m) {
  nb::class_<Desktop>(m, "Desktop")
      .def("create", &Desktop::create)
      .def("key_press", &Desktop::key_press)
      .def("key_release", &Desktop::key_release)
      .def("move_mouse", &Desktop::move_mouse)
      .def("mouse_press", &Desktop::mouse_press)
      .def("mouse_release", &Desktop::mouse_release)
      .def("get_frame", [](Desktop& d) {
        Frame f = d.get_frame();

        uint8_t* data = f.take_pixels().release();
        nb::capsule owner(data, [](void* p) noexcept { free((uint8_t*)p); });

        return nb::ndarray<uint8_t, nb::numpy, nb::shape<-1, -1, 4>,
                           nb::c_contig>(
            data, {(uint32_t)f.width, (uint32_t)f.height, 4}, owner);
      });
}
