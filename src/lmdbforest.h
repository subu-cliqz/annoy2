#ifndef __LMDB_FOREST_H
#define __LMDB_FOREST_H
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <memory.h>

#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>
#include <string>
#ifdef __MINGW32__
 #include "mman.h"
 #include <windows.h>
#else
 #include <sys/mman.h>
#endif


#include <vector>
#include <map>
#include "lmdb.h"

#include "annoylib.h"
#include "protobuf/annoy.pb.h"

#define E(expr) CHECK((rc = (expr)) == MDB_SUCCESS, #expr)
#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
#define CHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
"%s:%d: %s: %s\n", __FILE__, __LINE__, msg, mdb_strerror(rc)), abort()))


#define DBN_ROOT "root"
#define DBN_RAW "raw"
#define DBN_TREE "tree"


using namespace std;


/*
 a class to store a forest using key-value store LMDB. 
 
 1. Database DBN_RAW would use the id as the key for objs,
 store the raw vector values for each sample
 
 2. Database DBN_TREE would store the roots, the internal 
 nodes, as well as leaf nodes for the tree. 
 
    2.1 the keys for roots are "rt". the values are all the keys of the roots
    2.2 each node of the tree is stored as a protobuf obj tree_node
    2.3 leaf node would have an array of pointers to the raw data
 
 */

template<typename S, typename T>
class AnnoyIndexInterface {
public:
  virtual ~AnnoyIndexInterface() {};
  virtual void add_item(S item, const T* w) = 0;
  virtual void build(int q) = 0;
  virtual bool save(const char* filename) = 0;
  virtual void reinitialize() = 0;
  virtual void unload() = 0;
  virtual bool load(const char* filename) = 0;
  virtual T get_distance(S i, S j) = 0;
  virtual void get_nns_by_item(S item, size_t n, size_t search_k, vector<S>* result, vector<T>* distances) = 0;
  virtual void get_nns_by_vector(const T* w, size_t n, size_t search_k, vector<S>* result, vector<T>* distances) = 0;
  virtual S get_n_items() = 0;
  virtual void verbose(bool v) = 0;
  virtual void get_item(S item, vector<T>* v) = 0;


  virtual bool create()=0;
     
};

template<typename S, typename T, template<typename, typename, typename> class Distance, class Random>
class AnnoyIndex : public AnnoyIndexInterface<S, T> {

  protected:
    Random _random;
    typedef Distance<S, T, Random> D;
    bool _verbose;
    int rc; //for macro processisng
    
  protected:
    MDB_env* _env;
    MDB_dbi _dbi_raw;
    MDB_dbi _dbi_tree;
    MDB_txn* _txn;
  
    int _f ; // the dimension of data
    int _tree_count; //number of trees;
    int _K ; // maximum size of data in each leaf node

    vector<int> _roots;

    bool _read_only;



 public:


    AnnoyIndex(int f, int K, int r, const char* dir, int maxreaders, uint64_t maxsize, int read_only) : _random() {
      _f = f;
      _tree_count = r;
      _K = K;
      _verbose = false;

      //for lmdb usage
      _env = NULL;
      _txn = NULL;

      if (read_only == 1) {
        open_as_read(dir, maxreaders);
      } else {
        open_as_write(dir, maxreaders, maxsize);
      }
      _read_only = (read_only == 1);
      
    }
    
    ~AnnoyIndex(){
    }

    bool open_as_read(const char* database_directory, int maxreaders) {
      close_db();
      E(mdb_env_create(&_env));
      E(mdb_env_set_maxreaders(_env, maxreaders));
      E(mdb_env_set_maxdbs(_env, 100));
      if (_verbose)  { printf("opening db at %s ..", database_directory); fflush(stdout);}
      E(mdb_env_open(_env, database_directory, MDB_RDONLY, 0664));
      if (_verbose)  { printf("done.\n"); fflush(stdout);}
      for (int i = 0; i < _tree_count; i ++)
      {
        _roots.push_back(i);  
      }
      return true;
    }
    
    bool open_as_write(const char* database_directory, int maxreaders, uint64_t maxsize) {
      printf("init write dir: %s, maxreader %d, maxsize %lld\n", database_directory, maxreaders, maxsize);
      close_db();
      E(mdb_env_create(&_env));
      E(mdb_env_set_maxreaders(_env, maxreaders));
      E(mdb_env_set_mapsize(_env, maxsize));
      E(mdb_env_set_maxdbs(_env, 100));
      E(mdb_env_open(_env, database_directory, MDB_WRITEMAP, 0664));
      for (int i = 0; i < _tree_count; i ++)
      {
        _roots.push_back(i);  
      }
      return true;
    }

    bool create()
    {
      return init_roots();
    }
    
    bool close_db() {
      if (_env != NULL) {
          mdb_env_close(_env);
          _env = NULL;
      }
      return true;
    }
 

  

    void set_verbose(bool v) {
        _verbose = v;
    }
    
    //append data into this tree
  
    void add_item(S item, const T* w){
      
      data_info d;
      for(int i = 0; i < _f; i ++ ) {
        d.add_data(w[i]);
      }
      
      add_item(item, d);
      return;
    }
  
    void build(int q) {
      return;
    }
    
    bool save(const char* filename) { 
      return true; 
    }
    
    void reinitialize() {
      return;
    }
    
    void unload() {
      return;
    }
    
    bool load(const char* filename)  {
      return true;
    }
    
    T get_distance(S i, S j) {
      printf("calculating distance of data %d and %d\n", i,j);
 
      
      E(mdb_txn_begin(_env, NULL, MDB_RDONLY, &_txn));
      E(mdb_dbi_open(_txn, DBN_RAW, MDB_CREATE, &_dbi_raw));
      
      
      data_info di, dj;
      _get_raw_data(i, di);
      _get_raw_data(j, dj);
      printf("get raw data completed\n");
      fflush(stdout);

      di.PrintDebugString();
      dj.PrintDebugString();
      T dist =  D::distance(di, dj, _f);
      printf("distance is %f\n", dist);

      mdb_txn_abort(_txn);
      printf("completed\n");
      return dist;

    }


    void get_nns_by_item(S item, size_t n, size_t search_k, vector<S>* result, vector<T>* distances) {
      data_info d;
      _get_raw_data(item, d);
      

      return;
    }

    void get_nns_by_vector(const T* w, size_t n, size_t search_k, 
      vector<S>* result, vector<T>* distances) {
      printf("c++: get_nns_by_vector %d, %d\n", n, search_k);


      E(mdb_txn_begin(_env, NULL, MDB_RDONLY, &_txn));
      E(mdb_dbi_open(_txn, DBN_RAW, 0, &_dbi_raw));
      E(mdb_dbi_open(_txn, DBN_TREE, 0, &_dbi_tree));

       _get_all_nns(w, n, search_k, result, distances);
      mdb_txn_abort(_txn);
      
      return ;
    }

    void _get_all_nns(const T* v, size_t n, size_t search_k, vector<S>* result, vector<T>* distances) {
      
      std::priority_queue<pair<T, S> > q;

      if (search_k == (size_t) (-1)) {
        search_k = n * _tree_count; // slightly arbitrary default value
      }
      //printf("c++: get_nns_by_vector %d, %d\n", n, search_k);
    

      //put all root nodes in priority queue
      for (size_t i = 0; i < _tree_count; i++) {
        q.push(make_pair(numeric_limits<T>::infinity(), _roots[i]));
      }
    
      vector<S> nns;
     



      while (nns.size() < search_k && !q.empty()) {

        if (_verbose) {
          //printf("c++: get_nns_by_vector nns size %d, queue size : %d\n", nns.size(), q.size());
        }
    

        const pair<T, S>& top = q.top();
        T d = top.first;
        S i = top.second;
        tree_node tn;
        bool result = _get_node_by_index(i, tn);
        q.pop();
        if (tn.leaf()) {

          for (int i = 0; i < tn.items_size(); i ++) {
              nns.push_back(tn.items(i));
          }
        } else {

          //T margin = D::margin(nd, v, _f);
          T margin = D::margin(tn, v, _f);
          q.push(make_pair(std::min(d, +margin), tn.right()));
          q.push(make_pair(std::min(d, -margin), tn.left()));
        }
      }

      // Get distances for all items
      // To avoid calculating distance multiple times for any items, sort by id
      sort(nns.begin(), nns.end());
      vector<pair<T, S> > nns_dist;
      S last = -1;
      for (size_t i = 0; i < nns.size(); i++) {
        S j = nns[i];
        if (j == last)
          continue;
        last = j;

        data_info di;
        bool r = _get_raw_data(j, di);

        nns_dist.push_back(make_pair(D::distance(v, di,  _f), j));
      }

      size_t m = nns_dist.size();
      size_t p = n < m ? n : m; // Return this many items
      std::partial_sort(&nns_dist[0], &nns_dist[p], &nns_dist[m]);
      for (size_t i = 0; i < p; i++) {
        if (distances) {
          distances->push_back(D::normalized_distance(nns_dist[i].first));
        }
        result->push_back(nns_dist[i].second);
        if (_verbose) {
          printf("distance %d : -> %f\n", i,  nns_dist[i].first);
        }
      }

    }
     
  


    S get_n_items() {
      return 0;
    }
    
    void verbose(bool v){
      set_verbose(v);
    }
    
    void get_item(S item, vector<T>* v)  {
      return;
    };


    void add_item(int data_id, data_info& d) {

      E(mdb_txn_begin(_env, NULL, 0, &_txn));
      E(mdb_dbi_open(_txn, DBN_RAW, MDB_CREATE, &_dbi_raw));
      E(mdb_dbi_open(_txn, DBN_TREE, MDB_CREATE, &_dbi_tree));
      _add_raw_data(data_id, d);
      if (_verbose) {
        printf ("adding raw data \n");
        fflush(stdout);
      }
        
      for (int i = 0; i < _tree_count; i ++ ) {
        _add_item_to_tree(i, data_id);
      }
      mdb_txn_commit(_txn);
      return;
    }
  
    void _add_item_to_tree(int node_index, int data_id) {
      //check node type
      tree_node tn;
      bool result = _get_node_by_index(node_index, tn);
      if (!result)  {
        return;
      }

      if (tn.leaf() == true) {
          
        //check if it needs to split
        int size = tn.items_size() ;
        if (size + 1 <= _K) {
          tn.add_items(data_id);
          _update_tree_node(node_index, tn);
        } else {
            //split
          
          vector<data_info> data_pt;
          for (int k = 0; k < tn.items_size(); k ++) {
            data_info d;
            _get_raw_data(tn.items(k), d);
            data_pt.push_back(d);
          }
          tree_node new_node;
          tree_node left_node;
          tree_node right_node;
          D::split(tn, data_pt, new_node, left_node, right_node, _random, _f );
          _update_tree_node(node_index, new_node);
          _add_node(left_node);
          _add_node(right_node);
          
        }
      }
        
    }
    /*
    void get_roots() {
 
        MDB_val key, data;
        MDB_txn *txn;
        MDB_cursor *cursor;
        MDB_dbi dbi;
        
        E(mdb_txn_begin(_env, NULL, MDB_RDONLY, &txn));
        E(mdb_dbi_open(txn, DBN_ROOT, 0, &dbi));
        E(mdb_cursor_open(txn, dbi, &cursor));
        rc = mdb_cursor_get(cursor, &key, &data, MDB_FIRST);
        
        _roots.push_back(atoi((char*)data.mv_data));
        
        printf("%s\n", mdb_strerror(rc));
        if (rc == 0) {
            while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
                printf("get root %d with node id: %d \n", _roots.size(), atoi((char*)data.mv_data));      
                _roots.push_back(atoi((char*)data.mv_data));
            }
        }
        CHECK(rc == MDB_NOTFOUND, "mdb_cursor_get");
        mdb_dbi_close(_env, dbi);
        mdb_cursor_close(cursor);
        mdb_txn_abort(txn);
        
        return ;
        
    } */
    
  protected:
  
    // node 0, ..., _tree_count - 1 will be reserved for root nodes    
    bool init_roots() {
      
      MDB_val key, data;
      MDB_txn *txn;
      MDB_dbi dbi;
      bool success = true;
      
       _roots.clear();
      

      E(mdb_txn_begin(_env, NULL, 0 , &txn));
      MDB_dbi dbi_tree;
      E(mdb_dbi_open(txn, DBN_TREE, MDB_CREATE, &dbi_tree));
      for (int i = 0; i < _tree_count; i ++) {
        key.mv_data = (uint8_t*) & i;
        key.mv_size = sizeof(int);
        tree_node tn;
        tn.set_index(i);
        tn.set_leaf(true);
        tn.clear_left();
        tn.clear_right();
        string str;
        tn.SerializeToString(&str);
        data.mv_data = (uint8_t*)str.c_str();
        data.mv_size = str.length();
        int retval = mdb_put(txn, dbi_tree, &key, &data, 0);
        if (retval != MDB_SUCCESS) {
            printf("failed add root for tree %d, due to %s \n" , i, mdb_strerror(retval));
            fflush(stdout);
            success = false;
            break;
        }
        _roots.push_back(i);
      }
      if (success) {
        E(mdb_txn_commit(txn));
      } else {
        mdb_txn_abort(txn);
        _roots.clear();
      }
      return success;
    }
  
  
    int _get_max_tree_index() {
      
      
    }
    
    
    bool _add_node(tree_node & tn) {
        
        //get the largest index
        int max_index = _get_max_tree_index();
        return _update_tree_node(max_index + 1, tn);
    }
    
    bool _update_tree_node(int index, tree_node & tn) {
        int success = 0;
        MDB_val key, data;
        
        key.mv_data = (uint8_t*) & index;
        key.mv_size = sizeof(int);
        
        string data_buffer;
        tn.SerializeToString(&data_buffer);
        
        data.mv_size = data_buffer.length();
        data.mv_data = (uint8_t*)data_buffer.c_str();
        
        int retval = mdb_put(_txn, _dbi_tree, &key, &data, 0);
        
        
        if (retval == MDB_SUCCESS) {
            /*
             printf("successful to put %d, %d : key: %p %.*s, data: %p %.*s, due to : %s\n",
             (int) key.mv_size, (int) data.mv_size, key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
             data.mv_data, (int) data.mv_size, (char *) data.mv_data, mdb_strerror(retval));
             */
            success = 1;
        }
        else if (retval == MDB_KEYEXIST) {
            printf(" key/data pair is duplicated.\n");
            success = 0;
        } else {
            printf("failed to put %d, %d : key: %p %.*s, data: %p %.*s, due to : %s\n",
                   (int) key.mv_size, (int) data.mv_size, key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
                   data.mv_data, (int) data.mv_size, (char *) data.mv_data, mdb_strerror(retval));
            
            success = 0;
        }
        
        return success ;

    }
    
    bool _get_node_by_index(int index,  tree_node & tn ) {
        
        MDB_val key, data;
        key.mv_data = (uint8_t*) & index;
        key.mv_size = sizeof(int);
        rc = mdb_get(_txn, _dbi_tree, &key, &data);
        if (rc == 0) {
            string s_data((char*) data.mv_data, data.mv_size);
            tn.ParseFromString(s_data);
        } else {
            //printf("can not find raw image data with id: %d\n", image_id);
            return false;
        }
        return true;
        
    }

    
    bool _get_raw_data(int data_id,  data_info & rdata ) {
 
        MDB_val key, data;
        key.mv_data = (uint8_t*) & data_id;
        key.mv_size = sizeof(int);
        rc = mdb_get(_txn, _dbi_raw, &key, &data);
        if (rc == 0) {
            string s_data((char*) data.mv_data, data.mv_size);
            rdata.ParseFromString(s_data);
        } else {
            //printf("can not find raw image data with id: %d\n", image_id);
            return false;
        }
        return true;
        
    }
    
    
    int _add_raw_data(int data_id, data_info& rdata) {
        
        int success = 0;
        MDB_val key, data;
        
        key.mv_data = (uint8_t*) & data_id;
        key.mv_size = sizeof(int);
        
        string data_buffer;
        rdata.SerializeToString(&data_buffer);
        
        data.mv_size = data_buffer.length();
        data.mv_data = (uint8_t*)data_buffer.c_str();
        
        int retval = mdb_put(_txn, _dbi_raw, &key, &data, 0);
        
        
        if (retval == MDB_SUCCESS) {
            /*
             printf("successful to put %d, %d : key: %p %.*s, data: %p %.*s, due to : %s\n",
             (int) key.mv_size, (int) data.mv_size, key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
             data.mv_data, (int) data.mv_size, (char *) data.mv_data, mdb_strerror(retval));
             */
            success = 1;
        }
        else if (retval == MDB_KEYEXIST) {
            printf(" key/data pair is duplicated.\n");
            success = 0;
        } else {
            printf("failed to put %d, %d : key: %p %.*s, data: %p %.*s, due to : %s\n",
                   (int) key.mv_size, (int) data.mv_size, key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
                   data.mv_data, (int) data.mv_size, (char *) data.mv_data, mdb_strerror(retval));
            
            success = 0;
        }
        
        return success ;
        
    }
    
    
};



#endif
