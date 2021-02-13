#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <sys/stat.h>

#define HASH_SIZE 100

using namespace std;

struct Emp
{
  char id[8];
  char name[200];
  char bio[500];
  char manager_id[8];
};

struct Block
{
  int capacity;
  int used;
  int usage;                          // check which slot is in use, if usage = 0101 = 5, then the 1st and the 3th slot is used
  int overflow;
  char data[4080];
};


struct BucketArray
{
  int i;
  int N;
  int buckets[HASH_SIZE];
  int buckets_offsets[HASH_SIZE];
};

int         total_block     = 0;
int         mode            = 0;                     // 0 for Index creation mode / 1 for Look up mode
int         metaSize        = 16;                    // number of bytes for the meta data of a block
int         max_record_len  = sizeof(struct Emp);    // the max length of an record

struct Emp          emp_buffer;               // Emp buffer for reading file
struct Emp          temp_buffer;              // Emp temp buffer for moving record on split
string              line_buffer;              // line buffer for reading a single line from file
struct BucketArray  bucket_array;             // linear hash index

fstream     csv;                      // file pointer to read csv file
fstream     b_array;                  // file pointer to bucket_array
fstream     data;                     // file pointer for EmployeeIndex file

int            read_line();                                // read a line from the csv file and put data in to the Emp struct
int            hash_id( char *id );                        // hash the id and return the key
void           init_index();                               // init hash index
void           init_data();                                // init block
void           write_cur_record( int pos, int flag  );     // write the current emp_buffer to a block, pos is the entry of the block
void           print_record( int pos );                    // print the record on pos location
int            add_to_BucketArray( char *id );             // add the key to BucketArray, return entry to block
int            need_to_split();                            // return 1 if need to split, 0 otherwise
void           split();                                    // split and add one block
void           check_block(int entry, int i, int of);      // check each record in a overflow block and put them in right place
int            find_record_by_id( char *id );              // find the record offset by id
void           print_blocks();
void           print_bucket();


int main(int argc, char **argv)
{

  if ( argc != 2 || (strcmp(argv[1], "-C") != 0 && strcmp(argv[1], "-L")) )
  {
    cout << "Usage: main.out [-C/-L]     C: Index creation mode / L: Lookup mode" << endl;
    exit(0);
  }

  if ( strcmp(argv[1], "-C") != 0 )
    mode = 1;

  if ( mode == 0 )
  {
    csv.open("Employees.csv", ios::in);
    if ( !csv.is_open() )
    {
      cout << "Failed to open Employees.csv file" << endl;
      exit(1);
    }

    init_data();
    init_index();

    //int count = 0;
    while ( read_line() )
    {
      //cout << count << endl;
      //count++;

      if (need_to_split() == 1)
      {
        split();
      }
      int block_entry = add_to_BucketArray( emp_buffer.id );
      write_cur_record(block_entry, 0);

      //cout << "Added key: " << hash_id(emp_buffer.id) <<endl;
      //cout << "Adding to the " << block_entry / 4096 << "th block" << endl;
      //cout << "total_block: " << total_block << endl;
      //print_bucket();
      //print_blocks();
      //cout << endl << endl << endl;
    }
    csv.close();

    b_array.open("BucketArray", ios::out | ios::binary);
    b_array.write((char *)&bucket_array, sizeof(struct BucketArray));
    b_array.close();
    print_bucket();
    print_blocks();

    // char newone[] = "11432121";
    // int test = find_record_by_id(newone);
    // if (test == -1)
    //   cout << "Record not found" << endl;
    // else
    //   print_record(test);

  }
  else
  {
    b_array.open("BucketArray", ios::in| ios::binary);

    if (!b_array.is_open())
    {
      cout << "Can not open BucketArray file, please run with -C first" << endl;
      exit(1);
    }

    b_array.read((char *)&bucket_array, sizeof(struct BucketArray));
    b_array.close();

    data.open("EmployeeIndex", ios::in);
    if (!data.is_open())
    {
      cout << "Can not open EmployeeIndex file, please run with -C first" << endl;
      exit(1);
    }

    while(1)
    {
      char lookup_id[9];
      cout << "Enter the id of an record (ctrl-C to quit)" << endl;;
      cin.get(lookup_id, 9);
      cin.clear();
      cin.ignore(10000, '\n');
      cout <<  endl;
      int res = find_record_by_id(lookup_id);
      if (res == -1)
        cout << "Record not found, please check id" << endl;
      else
        print_record(res);
      cout << endl;
    }
  }

  return 0;
}

int read_line()
{
  if (getline(csv, line_buffer))
  {
    int i = 0;
    int c = 0;
    for ( i = 0; i < 8; i++ )                      // read id
    {
      emp_buffer.id[i] = line_buffer[c];
      c++;
    }

    c++;                                           //  skip ,

    for ( i = 0; line_buffer[c] != ','; i++)       //  read name
    {
      emp_buffer.name[i] = line_buffer[c];
      c++;
    }
    emp_buffer.name[i] = '\0';                     //  end of name

    c++;                                           //  skip ,

    for ( i = 0; line_buffer[c] != ','; i++)       //  read name
    {
      emp_buffer.bio[i] = line_buffer[c];
      c++;
    }
    emp_buffer.bio[i] = '\0';                      //  end of bio

    c++;                                           //  skip ,

    for ( i = 0; i < 8; i++ )                      // read manager_id
    {
      emp_buffer.manager_id[i] = line_buffer[c];
      c++;
    }

    return 1;
  }
  return 0;
}

int hash_id( char *id )
{
  return atoi(id) % HASH_SIZE;
}

void init_index ()
{
  bucket_array.i = 1;
  bucket_array.N = 1;
  for (int i = 0; i < HASH_SIZE; i++)
    bucket_array.buckets[i] = 0;
  bucket_array.buckets_offsets[0] = 0;
}

void init_data ()
{
  data.close();
  data.open("EmployeeIndex", ios::out | ios::binary);

  if ( !data.is_open() )
  {
    cout << "Failed to create EmployeeIndex file" << endl;
    exit(1);
  }

  struct Block new_block;
  new_block.capacity = 5;
  new_block.used = 0;
  new_block.usage = 0;
  new_block.overflow = 0;

  data.write((char *) &new_block, sizeof(struct Block));

  data.close();

  total_block = 1;
}

/*
  This function only handle putting current Emp data at pos.

  pos is nth byte from the beginning of the data file
  so it should be n * 4096.

  Return value will be entry of the current Emp data.

  ref https://courses.cs.vt.edu/cs2604/fall02/binio.html
*/
void write_cur_record ( int pos, int flag )
{
  data.close();
  data.open("EmployeeIndex", ios::in | ios::out | ios::binary);

  if ( !data.is_open() )
  {
    cout << "Failed to open EmployeeIndex file" << endl;
    exit(1);
  }

  if ( pos % 4096 != 0 || pos > 4096 * total_block )
  {
    cout << "Offset is not at the entry of a block" << endl;
    exit(1);
  }

  int capacity;
  int used;
  int usage;
  int overflow;

  data.seekg(pos);                                  // go to the meta-data
  data.read((char *)&capacity, sizeof(int));
  data.read((char *)&used, sizeof(int));
  data.read((char *)&usage, sizeof(int));
  data.read((char *)&overflow, sizeof(int));

  used++;
  data.close();
  data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
  data.seekp(pos + sizeof(int));                    // go to the used
  data.write((char *)&used, sizeof(int));           // update used

  int i;                                            // look for vaild slot
  for ( i = 0; i < capacity; i++ )
    if ( !(usage >> i & 1) )
      break;

  usage |= 1u << i;                                 // updata usage
  data.write((char *)&usage, sizeof(int));

  int write_pos = pos + metaSize + max_record_len * i;
  //cout << "Writing record at: " << write_pos << endl;
  data.close();
  data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
  data.seekp(write_pos);                            // go to the next empty slot
  if (flag == 1)
    data.write((char *)&temp_buffer, max_record_len);
  else
    data.write((char *)&emp_buffer, max_record_len);

  data.close();
}

void print_record( int pos )
{
  data.close();
  data.open("EmployeeIndex", ios::in | ios::out | ios::binary);

  if ( !data.is_open() )
  {
    cout << "Failed to open EmployeeIndex file" << endl;
    exit(1);
  }

  int file_size;
  struct stat results;
  if (stat("EmployeeIndex", &results) == 0)
    file_size = results.st_size;
  else
  {
    cout << "Can not read EmployeeIndex stat" << endl;
    exit(1);
  }

  if ( pos > file_size )
  {
    cout << "Offset is not valid" << endl;
    exit(1);
  }

  struct Emp temp;
  char  ids[9];
  ids[8] = '\0';

  data.seekg( pos );
  //cout << "Record at: " << pos <<endl;
  data.read((char *)&temp, sizeof(struct Emp));

  for ( int i = 0; i < 8; i++ )
    ids[i] = temp.id[i];

  cout << "-----------------------------------------------------" << endl;
  cout << "id: " << ids << endl << endl;

  cout << "name: " << temp.name << endl << endl;

  cout << "bio: " << temp.bio << endl << endl;

  for ( int i = 0; i < 8; i++ )
    ids[i] = temp.manager_id[i];

  cout << "manager_id: " << ids << endl;
  cout << "-----------------------------------------------------" << endl;

  data.close();
}

int need_to_split()
{
  int total_used = 0;
  int total_capacity = 0;

  for (int i = 0; i < total_block; i++)
  {
    int cur_block_capa;
    int cur_block_use;
    data.close();
    data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
    data.seekg( i * 4096);
    data.read((char *)&cur_block_capa, 4);
    data.read((char *)&cur_block_use, 4);
    total_capacity += cur_block_capa;
    total_used += cur_block_use;
  }

  float average = (float)total_used / (float)(bucket_array.N * 5);
  //cout << "Average usage: " << total_used << " / " << total_capacity << " = " << average << endl;
  if (average > 0.8001)
    return 1;

  return 0;
}

void check_block( int entry, int i, int of )
{
  int block_usage;

  data.close();
  data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
  data.seekg( 4096* entry + 4 + 4 );
  data.read((char *)&block_usage, 4);

  for (int j = 0; j < 5; j++)
  {
    if ((block_usage >> j) & 1)
    {
      char rec_id[8];
      data.close();
      data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
      data.seekg(4096* entry + metaSize + j * sizeof(struct Emp));
      data.read((char *)&rec_id, 8);
      //cout << "check_block_id: "<< rec_id << endl;

      int rec_key = hash_id(rec_id);
      //cout << "check_block_hash: "<< rec_key << endl;

      int idx = 0;
      for (int k = 0; k < bucket_array.i; k++)
        if ( (rec_key >> k) & 1 )
          idx |= (1u << k);

      //cout << "check_block_idx: "<< idx << endl;


      if (idx >= bucket_array.N)
        idx ^= 1u << (bucket_array.i - 1);

      //cout << "check_block_idx: "<< idx << endl;

      //cout << "check_block: idx   i  " << idx << "  " << i <<endl;

      if (idx == i)
        continue;
      else
      {
        if (!of)
          bucket_array.buckets[i] -= 1;
        block_usage &= ~(1 << j);
        data.close();
        data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
        data.seekp( 4096 * entry + 8);
        data.write((char *)&block_usage, 4);
        int block_used;
        data.close();
        data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
        data.seekg( 4096 * entry + 4);
        data.read((char *)&block_used, 4);
        block_used -= 1;
        data.close();
        data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
        data.seekp( 4096 * entry + 4);
        data.write((char *)&block_used, 4);

        data.close();
        data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
        data.seekg(4096* entry + metaSize + j * sizeof(struct Emp));
        data.read((char *)&temp_buffer, sizeof(struct Emp));
        //cout << "check_block: temp_buffer_id: ";
        //for (int z = 0; z < 8; z++)
        //  cout << temp_buffer.id[z];
        //cout << endl;
        int block_entry = add_to_BucketArray( temp_buffer.id );
        write_cur_record(block_entry, 1);
      }
    }
  }
  data.close();
}

void split()
{
  //cout << "Need to split" << endl;
  struct Block new_block;
  new_block.capacity = 5;
  new_block.used = 0;
  new_block.usage = 0;
  new_block.overflow = 0;

  data.close();
  data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
  data.seekp(total_block * sizeof(struct Block));
  data.write((char *)&new_block, sizeof(struct Block));

  bucket_array.buckets_offsets[bucket_array.N] = total_block;
  bucket_array.N += 1;
  total_block += 1;

  //cout << "After add new block: " << endl;

  int max_N = 1;
  for (int i = 0; i < bucket_array.i; i++)
    max_N *= 2;

  if (bucket_array.N > max_N)
    bucket_array.i += 1;

  //print_bucket();
  //print_blocks();

  for (int i = 0; i < bucket_array.N; i++)
  {
    int entry = bucket_array.buckets_offsets[i];
    check_block(entry, i, 0);
    //cout << "After check non overflow block " << entry << endl;
    //print_bucket();
    //print_blocks();

    int cur_overflow;
    data.close();
    data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
    data.seekg( 4096* entry + 4 + 4 + 4);
    data.read((char *)&cur_overflow, 4);

    while (cur_overflow != 0)
    {
      check_block(cur_overflow, i, 1);
      //cout << "After check overflow block " << cur_overflow << endl;
      //print_bucket();
      //print_blocks();

      data.close();
      data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
      data.seekg( 4096* cur_overflow + 4 + 4 + 4);
      data.read((char *)&cur_overflow, 4);
    }
  }
  data.close();
}

int add_to_BucketArray( char *id )
{
  int key = hash_id(id);

  int idx = 0;
  for (int i = 0; i < bucket_array.i; i++)
    if ( (key >> i) & 1 )
      idx |= (1u << i);

  //cout << "_idx: " << idx << endl;

  if (idx >= bucket_array.N)
  {
    idx ^= 1u << (bucket_array.i - 1);
    //cout << "bit flip, idx: " << idx << endl;
  }

  if (idx < bucket_array.N)
  {
    //print_bucket();
    if (bucket_array.buckets[idx] < 5)
    {
      bucket_array.buckets[idx] += 1;
      return bucket_array.buckets_offsets[idx] * 4096;
    }
    else
    {
      //cout << "add_to_BucketArray: need to go to overflow block" << endl;
      int overflow_entry;
      int cur_block = bucket_array.buckets_offsets[idx];
      data.close();
      data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
      data.seekg(cur_block * 4096 + 12);
      data.read((char *)&overflow_entry, 4);
      //cout << "add_to_BucketArray current block: " << cur_block << endl;
      //cout << "add_to_BucketArray overflow_entry: " << overflow_entry << endl;
      while (1)
      {
        if (overflow_entry == 0)
        {
          //cout << "add_to_BucketArray: need to add a new block to "<< bucket_array.buckets_offsets[idx] << endl;
          int overflow = total_block;

          struct Block new_block;
          new_block.capacity = 5;
          new_block.used = 0;
          new_block.usage = 0;
          new_block.overflow = 0;

          data.close();
          data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
          data.seekp(total_block * sizeof(struct Block));
          data.write((char *)&new_block, sizeof(struct Block));
          total_block++;

          data.close();
          data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
          data.seekp(cur_block * 4096 + 12);
          data.write((char *)&overflow, 4);
          data.close();
          //cout << "Add to new overflow block: " << overflow << endl;
          return overflow * 4096;
        }
        else
        {
          int block_used;
          cur_block = overflow_entry;
          data.close();
          data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
          data.seekg(cur_block * sizeof(struct Block) + 4);
          data.read((char *)&block_used, 4);
          data.close();
          data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
          data.seekg(cur_block * sizeof(struct Block) + 12);
          data.read((char *)&overflow_entry, 4);
          //cout << "go to cur_block: " << cur_block << endl;
          //cout << "block used: " << block_used << endl;
          //cout << "overflow_entry: " << overflow_entry << endl;

          if (block_used < 5)
          {
            //cout << "Add to overflow block: " << cur_block << endl;
            data.close();
            return cur_block * 4096;
          }
        }
      }
    }
  }
  return -1;
}

int find_record_by_id( char *id )
{
  int key = hash_id(id);
  int idx = 0;

  for (int i = 0; i < bucket_array.i; i++)
    if ( (key >> i) & 1 )
      idx |= (1u << i);

  //cout << idx << endl;

  if (idx >= bucket_array.N)
  {
    idx ^= 1u << (bucket_array.i - 1);
  }
  int entry = bucket_array.buckets_offsets[idx];

  //cout << idx << endl;
  //cout << entry << endl;

  while(1)
  {
    //cout << entry << endl;

    int usage;
    data.close();
    data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
    data.seekg(entry * 4096 + 8);
    data.read((char *)&usage, 4);

    for (int i = 0; i < 5; i++)
    {
      if ((usage >> i) & 1)
      {
        int match = 1;
        char cur_id[8];
        int cur_id_offset = entry * 4096 + metaSize + i * max_record_len;
        data.close();
        data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
        data.seekg(cur_id_offset);
        data.read((char *)&cur_id, 8);
        //cout << cur_id << endl;

        for( int j = 0; j < 8; j++)
          if (cur_id[j] != id[j])
            match = 0;

        if (match)
        {
          data.close();
          return cur_id_offset;
        }
      }
    }
    data.close();
    data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
    data.seekg(entry * 4096 + 12);
    data.read((char *)&entry, 4);
    if (entry == 0)
      return -1;
  }
  data.close();
  return -1;
}

void print_blocks()
{
  for( int i = 0; i < total_block; i++)
  {
    cout << "Block " << i <<endl;
    int block_capacity;
    int block_used;
    int block_usage;
    int overflow;
    data.close();
    data.open("EmployeeIndex", ios::in | ios::out | ios::binary);
    data.seekg(i * 4096);
    data.read((char *)&block_capacity, 4);
    data.read((char *)&block_used, 4);
    data.read((char *)&block_usage, 4);
    data.read((char *)&overflow, 4);
    cout << "Capacity: " << block_capacity << endl;
    cout << "Used: " << block_used << endl;
    cout << "Usage: " << block_usage << endl;
    cout << "Overflow: " << overflow << endl <<endl;
    cout << "Block data:" << endl;
    for (int j = 0; j < 5; j++)
    {
      if ( (block_usage >> j) & 1 )
      {
        int where = i*4096 + metaSize + j * sizeof(struct Emp);
        print_record( where );
      }
    }
  }
  cout << endl;
  data.close();
}

void print_bucket()
{
  cout << "Bucket: " << endl;
  cout << "i: " << bucket_array.i << endl;
  cout << "N: " << bucket_array.N << endl;
  for (int i = 0; i < bucket_array.N; i++)
  {
    cout << "Bucket " << i << " size: " << bucket_array.buckets[i] << endl;
    cout << "Bucket " << i << " pointer: " << bucket_array.buckets_offsets[i] << endl;
  }
  cout << endl;
}
