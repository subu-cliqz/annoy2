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

    def tes1t_set_root(self):
        print "test_set_root"
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 3
        i = AnnoyIndex(f, 3, "test_db", 10, 1000, 3048576000, 0)
        #i.verbose(True)
        i.create()
        for k in range(10):
            i.display_node(k)

        i.add_item(0, [0, 0, 1])
        print "after adding 1 data"
        for k in range(10):
            i.display_node(k)

        i.add_item(1, [0, 1, 0])
        print "after adding 2 data"
        for k in range(10):
            i.display_node(k)

        i.add_item(2, [1, 0, 0])
        print "after adding 3 data"
        for k in range(10):
            i.display_node(k)

        print "get nns by vector [3,2,1]"
        print i.get_nns_by_vector([3, 2, 1], 3)
        self.assertEqual(i.get_nns_by_vector([3, 2, 1], 3), [2, 1, 0])
        self.assertEqual(i.get_nns_by_vector([1, 2, 3], 3), [0, 1, 2])
        self.assertEqual(i.get_nns_by_vector([2, 0, 1], 3), [2, 0, 1])

        print "create i2"
        i2 = AnnoyIndex(f, 3, "test_db", 10, 1000, 3048576000, 1)
        i2.verbose(True)
        self.assertEqual(i2.get_nns_by_vector([3, 2, 1], 3), [2, 1, 0])
        self.assertEqual(i2.get_nns_by_vector([1, 2, 3], 3), [0, 1, 2])
        self.assertEqual(i2.get_nns_by_vector([2, 0, 1], 3), [2, 0, 1])

    def test_get_n_items(self):
        print "test_get_n_items"
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 3
        i = AnnoyIndex(f, 2, "test_db", 10, 1000, 3048576000, 0)
        #i.verbose(True)
        i.create()

        i.add_item(0, [0, 0, 1])
        self.assertEqual(i.get_n_items(), 1);
        i.add_item(1, [0, 1, 0])
        self.assertEqual(i.get_n_items(), 2);
        i.add_item(2, [1, 0, 0])
        self.assertEqual(i.get_n_items(), 3);
     
    def test_get_item(self):
        print "test_get_item"
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 3
        i = AnnoyIndex(f, 2, "test_db", 10, 1000, 3048576000, 0)
        i.create()

        i.add_item(0, [0, 0, 1])
        i.add_item(1, [0, 1, 0])
        i.add_item(2, [1, 0, 0])
        i1 = i.get_item(0);
        self.assertEqual(i1, [0, 0, 1]);
        i2 = i.get_item(1);
        self.assertEqual(i2, [0, 1, 0]);
        i3 = i.get_item(2);
        self.assertEqual(i3, [1, 0, 0]);


    def test1_add_item_1(self):
        print "test_set_root"
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 3
        i = AnnoyIndex(f, 2, "test_db", 10, 1000, 3048576000, 0)
        #i.verbose(True)
        i.create()
        for k in range(10):
            i.display_node(k)

        i.add_item(0, [0, 0, 1])
        print "after adding 1 data"
        for k in range(10):
            i.display_node(k)

        i.add_item(1, [0, 1, 0])
        print "after adding 2 data"
        for k in range(10):
            i.display_node(k)

        i.add_item(2, [1, 0, 0])
        print "after adding 3 data"
        for k in range(100):
            print "node %d" % k
            i.display_node(k)

       

        print "get nns by vector [3,2,1]"
        self.assertEqual(i.get_nns_by_vector([3, 2, 1], 3), [2, 1, 0])
        self.assertEqual(i.get_nns_by_vector([1, 2, 3], 3), [0, 1, 2])
        self.assertEqual(i.get_nns_by_vector([2, 0, 1], 3), [2, 0, 1])




if __name__ == '__main__':
    unittest.main()
