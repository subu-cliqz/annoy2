#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
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

from setuptools import setup, Extension
import codecs
import os
import sys

readme_note = """\
.. note::

   For the latest source, discussion, etc, please visit the
   `GitHub repository <https://github.com/spotify/annoy>`_\n\n

.. image:: https://img.shields.io/github/stars/spotify/annoy.svg
    :target: https://github.com/spotify/annoy

"""

with codecs.open('README.rst', encoding='utf-8') as fobj:
    long_description = readme_note + fobj.read()


setup(name='annoy',
      version='2.0.0',
      description='Approximate Nearest Neighbors in C++/Python optimized for memory usage and loading/saving to disk.',
      packages=['annoy'],
      ext_modules=[
        Extension(
            'annoy.annoylib', ['src/annoymodule.cc',  'src/protobuf/annoy.pb.cc'],
            depends=['src/annoylib.h', 'src/lmdbforest.h', 'src/protobuf/annoy.pb.h'],
            include_dirs=['src', '/usr/loca/include', '/opt/local/include', '/usr/local/Cellar/protobuf/2.6.0/include/'],
            extra_compile_args=['-O3', '-march=native', '-std=c++11', '-ffast-math'],
            libraries = ["lmdb", "protobuf"]
        )
      ],
      long_description=long_description,
      author='Erik Bernhardsson',
      author_email='mail@erikbern.com',
      url='https://github.com/spotify/annoy',
      license='Apache License 2.0',
      classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Programming Language :: Python',
          'Programming Language :: Python :: 2.6',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 3.3',
          'Programming Language :: Python :: 3.4',
      ],
      keywords='nns, approximate nearest neighbor search',
      setup_requires=['nose>=1.0']
    )
