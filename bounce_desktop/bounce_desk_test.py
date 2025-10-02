import unittest

from bounce_desktop import Desktop


class TestDesktop(unittest.TestCase):
    def test_get_frame(self):
        d = Desktop.create(300, 200, ["sleep", "10000"])
        frame = d.get_frame()
        self.assertEqual(frame.shape, (300, 200, 4))


if __name__ == "__main__":
    unittest.main()
