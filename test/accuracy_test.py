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

from __future__ import print_function

import unittest
import random
import os
from annoy import AnnoyIndex
try:
    from urllib import urlretrieve
except ImportError:
    from urllib.request import urlretrieve # Python 3
import gzip
from nose.plugins.attrib import attr

class AccuracyTest(unittest.TestCase):
    def _get_index(self, f, distance):
        input = 'test/glove.twitter.27B.%dd.txt.gz' % f
        output = 'test/glove.%d.%s.annoy' % (f, distance)
        
        if not os.path.exists(output):
            if not os.path.exists(input):
                # Download GloVe pretrained vectors: http://nlp.stanford.edu/projects/glove/
                # Hosting them on my own S3 bucket since the original files changed format
                url = 'https://s3-us-west-1.amazonaws.com/annoy-vectors/glove.twitter.27B.%dd.txt.gz' % f
                print('downloading', url, '->', input)
                urlretrieve(url, input)

            print('building index', distance, f)
            annoy =  AnnoyIndex(f, 12, "test_db", 10,  1000, 3048576000, 0)
            v_v = []
            items = []
            for i, line in enumerate(gzip.open(input, 'rb')):
                v = [float(x) for x in line.strip().split()[1:]]
                v_v.append(v)
                items.append(i)
                if (i+1) % 10000 == 0:
                    print (i+1)
                    annoy.add_item_batch(items, v_v)
                    v_v = []
                    items = []
            if v_v:
                annoy.add_item_batch(items, v_v)
        return annoy

    def _test_index(self, f, distance, exp_accuracy):
        annoy = self._get_index(f, distance)

        n, k = 0, 0

        for i in range(10000):
            js_fast = annoy.get_nns_by_item(i, 11, 1000)[1:]
            js_slow = annoy.get_nns_by_item(i, 11, 10000)[1:]
            assert len(js_fast) == 10
            assert len(js_slow) == 10

            n += 10
            k += len(set(js_fast).intersection(js_slow))

        accuracy = 100.0 * k / n
        print('%20s %4d accuracy: %5.2f%% (expected %5.2f%%)' % (distance, f, accuracy, exp_accuracy))

        self.assertTrue(accuracy > exp_accuracy - 1.0) # should be within 1%

    def test_angular_25(self):
        self._test_index(25, 'angular', 46.80)

    @attr('slow')
    def test_angular_50(self):
        self._test_index(50, 'angular', 31.00)

    @attr('slow')
    def test_angular_100(self):
        self._test_index(100, 'angular', 24.50)

    @attr('slow')
    def test_angular_200(self):
        self._test_index(200, 'angular', 15.18)

if __name__ == '__main__':
    os.system("rm -rf test_db")
    os.system("mkdir test_db")
    unittest.main()
    os.system("rm -rf test_db")