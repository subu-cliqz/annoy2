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
        print "test_dist "
        f = 2
        print "creating object"
        
        i = AnnoyIndex(f,  2, "test_db", 64,  1000, 3048576000)
        
        print "creating object"
        i.add_item(0, [0, 1])
        i.add_item(1, [1, 1])

        print "creating object"
        self.assertAlmostEqual(i.get_distance(0, 1), 2 * (1.0 - 2 ** -0.5))
        print "done"


if __name__ == '__main__':
    unittest.main()
