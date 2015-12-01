# Copyright (c) 2013 Spotify AB
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

# TeamCity fails to run this test because it can't import the C++ module.
# I think it's because the C++ part gets built in another directory.

import unittest
import random
import numpy
from annoy import AnnoyIndex
import os
import math
import time
try:
    xrange
except NameError:
    # Python 3 compat
    xrange = range


class TestCase(unittest.TestCase):
    def assertAlmostEquals(self, x, y):
        # Annoy uses float precision, so we override the default precision
        super(TestCase, self).assertAlmostEquals(x, y, 4)


class AngularIndexTest(TestCase):

    def test_dist(self):
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        
        f = 2   
        i = AnnoyIndex(f,  2, "test_db", 64,  1000, 3048576000, 0)
        # i.verbose(True)
        i.add_item(0, [0, 1])
        i.add_item(1, [1, 1])

        self.assertAlmostEqual(i.get_distance(0, 1), 2 * (1.0 - 2 ** -0.5))

    def test_dist_2(self):
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 2
        i = AnnoyIndex(f, 2, "test_db", 64,  1000, 3048576000, 0)
        i.add_item(0, [1000, 0])
        i.add_item(1, [10, 0])

        self.assertAlmostEqual(i.get_distance(0, 1), 0)

    def test_dist_3(self):
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 2
        i = AnnoyIndex(f, 2, "test_db", 64,  1000, 3048576000, 0)
        i.add_item(0, [97, 0])
        i.add_item(1, [42, 42])

        dist = (1 - 2 ** -0.5) ** 2 + (2 ** -0.5) ** 2

        self.assertAlmostEqual(i.get_distance(0, 1), dist)

    def test_dist_degen(self):
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 2
        i = AnnoyIndex(f, 2, "test_db", 64,  1000, 3048576000, 0)
        
        i.add_item(0, [1, 0])
        i.add_item(1, [0, 0])

        self.assertAlmostEqual(i.get_distance(0, 1), 2.0)
    def test_get_nns_by_vector(self):
        print "test_get_nns_by_vector "
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 3
        i = AnnoyIndex(f, 3, "test_db", 10, 1000, 3048576000, 0)
        i.add_item(0, [0, 0, 1])
        i.add_item(1, [0, 1, 0])
        i.add_item(2, [1, 0, 0])
      

        self.assertEqual(i.get_nns_by_vector([3, 2, 1], 3), [2, 1, 0])
        self.assertEqual(i.get_nns_by_vector([1, 2, 3], 3), [0, 1, 2])
        self.assertEqual(i.get_nns_by_vector([2, 0, 1], 3), [2, 0, 1])

    def test_get_nns_by_item(self):
        print "test_get_nns_by_item "
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 3
        i = AnnoyIndex(f, 3, "test_db", 10, 1000, 3048576000, 0)
        i.add_item(0, [2, 1, 0])
        i.add_item(1, [1, 2, 0])
        i.add_item(2, [0, 0, 1])
       
        self.assertEqual(i.get_nns_by_item(0, 3), [0, 1, 2])
        self.assertEqual(i.get_nns_by_item(1, 3), [1, 0, 2])
        self.assertTrue(i.get_nns_by_item(2, 3) in [[2, 0, 1], [2, 1, 0]]) # could be either

    def test_get_nns_by_item_batch(self):
        print "test_get_nns_by_item_batch "
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        f = 3
        i = AnnoyIndex(f, 3, "test_db", 10, 1000, 3048576000, 0)
        i.add_item_batch([0,1,2], [[2, 1, 0], [1, 2, 0], [0, 0, 1]])
       
        self.assertEqual(i.get_nns_by_item(0, 3), [0, 1, 2])
        self.assertEqual(i.get_nns_by_item(1, 3), [1, 0, 2])
        self.assertTrue(i.get_nns_by_item(2, 3) in [[2, 0, 1], [2, 1, 0]]) # could be either

    def test_large_index(self):
        print "test_large_index"
        start_time = int(round(time.time() * 1000))

        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        # Generate pairs of random points where the pair is super close
        f = 100
        i = AnnoyIndex(f, 12, "test_db", 10,  1000, 3048576000, 0)
        # i.verbose(True)
        for j in xrange(0, 100, 2):
            p = [random.gauss(0, 1) for z in xrange(f)]
            f1 = random.random() + 1
            f2 = random.random() + 1
            x = [f1 * pi + random.gauss(0, 1e-2) for pi in p]
            y = [f2 * pi + random.gauss(0, 1e-2) for pi in p]
            i.add_item(j, x)

            i.add_item(j+1, y)
        i = AnnoyIndex(f, 12, "test_db", 10,  1000, 3048576000, 1)
        for j in xrange(0, 100, 2):
            self.assertEqual(i.get_nns_by_item(j, 2, 50), [j, j+1])
            self.assertEqual(i.get_nns_by_item(j+1, 2, 50), [j+1, j])
        print "Total time = ",  (int(round(time.time() * 1000)) - start_time)/1000
            
    def t1est_large_index_batch(self):
        print "test_large_index_batch"
        start_time = int(round(time.time() * 1000))
        os.system("rm -rf test_db")
        os.system("mkdir test_db")
        # Generate pairs of random points where the pair is super close
        f = 100
        i = AnnoyIndex(f, 12, "test_db", 10,  1000, 3048576000, 0)
        i_v = []
        v_v = []
        for j in xrange(0, 100000, 2):
            p = [random.gauss(0, 1) for z in xrange(f)]
            f1 = random.random() + 1
            f2 = random.random() + 1
            x = [f1 * pi + random.gauss(0, 1e-2) for pi in p]
            y = [f2 * pi + random.gauss(0, 1e-2) for pi in p]
            i_v.append(j)
            i_v.append(j+1)
            v_v.append(x)
            v_v.append(y)
        
        i.add_item_batch(i_v, v_v)

        i = AnnoyIndex(f, 12, "test_db", 10,  1000, 3048576000, 1)
        for j in xrange(0, 100000, 2):
            self.assertEqual(i.get_nns_by_item(j, 2, 50), [j, j+1])
            self.assertEqual(i.get_nns_by_item(j+1, 2, 50), [j+1, j])
        print "Total time = ",  (int(round(time.time() * 1000)) - start_time)/1000

    def precision(self, n, n_trees=10, n_points=10000, n_rounds=10):
        found = 0
        for r in xrange(n_rounds):
            os.system("rm -rf test_db")
            os.system("mkdir test_db")
            # create random points at distance x from (1000, 0, 0, ...)
            f = 10
            i = AnnoyIndex(f, 10, "test_db", n_trees, 1000, 3048576000, 0)
            for j in xrange(n_points):
                p = [random.gauss(0, 1) for z in xrange(f - 1)]
                norm = sum([pi ** 2 for pi in p]) ** 0.5
                x = [1000] + [pi / norm * j for pi in p]
                i.add_item(j, x)

            nns = i.get_nns_by_vector([1000] + [0] * (f-1), n)
            self.assertEqual(nns, sorted(nns))  # should be in order
            # The number of gaps should be equal to the last item minus n-1
            found += len([x for x in nns if x < n])

        return 1.0 * found / (n * n_rounds)

    def test_precision_1(self):
        print "test_precision_1"
        res = self.precision(1)
        print res
        self.assertTrue(res >= 0.98)

    def test_precision_10(self):
        print "test_precision_10"
        res = self.precision(10)
        print res
        self.assertTrue(res >= 0.98)

    def test_precision_100(self):
        print "test_precision_100"
        res = self.precision(100)
        print res
        self.assertTrue(res >= 0.98)

    def test_precision_1000(self):
        print "test_precision_1000"    
        res = self.precision(1000)
        print res
        self.assertTrue(res >= 0.98)
        
    def test_precision_10000(self):
        print "test_precision_1000"    
        res = self.precision(10000)
        print res
        self.assertTrue(res >= 0.98)
if __name__ == '__main__':
    unittest.main()
