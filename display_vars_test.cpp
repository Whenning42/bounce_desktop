#include "display_vars.h"

#include <gtest/gtest.h>

TEST(DisplayVars, read_write_vars_round_trip) {
  setenv("DISPLAY", ":5", true);
  setenv("WAYLAND_DISPLAY", "wayland-2", true);
  const std::string instance_name = "test_instance";
  write_vars(instance_name);
  DisplayVars vars;
  bool read = read_vars(instance_name, &vars);
  EXPECT_TRUE(read);
  EXPECT_EQ(vars.x_display, ":5");
  EXPECT_EQ(vars.wayland_display, "wayland-2");
  clean_up_vars(instance_name);
}

TEST(DisplayVars, clean_up_deletes_vars) {
  setenv("DISPLAY", ":5", true);
  setenv("WAYLAND_DISPLAY", "wayland-2", true);
  const std::string instance_name = "delete_test_instance";
  write_vars(instance_name);
  DisplayVars vars;
  clean_up_vars(instance_name);
  EXPECT_FALSE(read_vars(instance_name, &vars));
}
