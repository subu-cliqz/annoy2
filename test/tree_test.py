import unittest
import random
import numpy
from annoy import AnnoyIndex
import os

class TestCase(unittest.TestCase):
    def assertAlmostEquals(self, x, y):
        # Annoy uses float precision, so we override the default precision
        super(TestCase, self).assertAlmostEquals(x, y, 4)


class TreeTest(TestCase):

    def test_set_root(self):
        print "test_set_root"
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 3
        i = AnnoyIndex(f, 3, "test_db", 10, 1000, 3048576000, 0)
        i.create()


        i.add_item(0, [0, 0, 1])
        i.add_item(1, [0, 1, 0])
        i.add_item(2, [1, 0, 0])
      

        self.assertEqual(i.get_nns_by_vector([3, 2, 1], 3), [2, 1, 0])
        self.assertEqual(i.get_nns_by_vector([1, 2, 3], 3), [0, 1, 2])
        self.assertEqual(i.get_nns_by_vector([2, 0, 1], 3), [2, 0, 1])


if __name__ == '__main__':
    unittest.main()
