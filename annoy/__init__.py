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

from .annoylib import *

class AnnoyIndex(Annoy):
    def __init__(self, f, K, file_dir, r,  max_reader, max_size,  read_only, metric='angular'):
        """
        :param metric: 'angular' or 'euclidean'
        """
        self.f = f
        
        super(AnnoyIndex, self).__init__(f, K, file_dir, r, max_reader, max_size, read_only, metric)

    def check_list(self, vector):
        if type(vector) != list:
            vector = list(vector)
        # if len(vector) != self.f:
        #     raise IndexError('Vector must be of length %d' % self.f)
        return vector

    def add_item(self, i, vector):
        # Wrapper to convert inputs to list
        return super(AnnoyIndex, self).add_item(i, self.check_list(vector))
 
    def add_item_batch(self, item_vector, vectors_vector):
        # Wrapper to convert inputs to list
        return super(AnnoyIndex, self).add_item_batch(self.check_list(item_vector), self.check_list(vectors_vector))

    def get_nns_by_vector(self, vector, n, search_k=-1, include_distances=False):
        # same
        return super(AnnoyIndex, self).get_nns_by_vector(self.check_list(vector), n, search_k, include_distances)

    def get_nns_by_item(self, item, n, search_k=-1, include_distances=False):
        # same
        return super(AnnoyIndex, self).get_nns_by_item(item, n, search_k, include_distances)
        
    def get_n_items(self):
        # same
        return super(AnnoyIndex, self).get_n_items()
        
    def get_item(self, item):
        # same
        return super(AnnoyIndex, self).get_item_vector(item)