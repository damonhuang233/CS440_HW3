#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <sys/stat.h>

#define HASH_SIZE 100;

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

int         mode            = 0;                     // 0 for Index creation mode / 1 for Look up mode
int         metaSize        = 16;                    // number of bytes for the meta data of a block
int         max_record_len  = sizeof(struct Emp);    // the max length of an record

struct Emp  emp_buffer;               // Emp buffer for reading file
string      line_buffer;              // line buffer for reading a single line from file

fstream     csv;                      // file pointer to read csv file
fstream     idx;                      // file pointer for EmployeeIndex file
fstream     data;                     // file pointer for EmployeeData file

int         read_line();                      // read a line from the csv file and put data in to the Emp struct
int         hash_id( char *id );              // hash the id and return the key
void        init_index();                     // init hash index
void        init_data();                      // init block
int         write_cur_record( int pos );      // write the current emp_buffer to a block, pos is the entry of the block

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

    while ( read_line() )
    {
      int key = hash_id(emp_buffer.id);
    }

    int offset = write_cur_record(0);

    csv.close();
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
  // In progress.........
}

void init_data ()
{
  data.open("EmployeeData", ios::out | ios::binary);

  if ( !data.is_open() )
  {
    cout << "Failed to create EmployeeData file" << endl;
    exit(1);
  }

  struct Block new_block;
  new_block.capacity = 4;
  new_block.used = 0;
  new_block.usage = 0;
  new_block.overflow = 0;

  data.write((char *) &new_block, sizeof(struct Block));

  data.close();
}

/*
  This function only handle putting current Emp data at pos.

  pos is nth byte from the beginning of the data file
  so it should be n * 4096.

  Return value will be entry of the current Emp data.

  ref https://courses.cs.vt.edu/cs2604/fall02/binio.html
*/
int write_cur_record ( int pos )
{
  data.open("EmployeeData", ios::in | ios::out | ios::binary);

  if ( !data.is_open() )
  {
    cout << "Failed to open EmployeeData file" << endl;
    exit(1);
  }

  int file_size;
  struct stat results;
  if (stat("EmployeeData", &results) == 0)
    file_size = results.st_size;
  else
  {
    cout << "Can not read EmployeeData stat" << endl;
    exit(1);
  }

  if ( pos % 4096 != 0 || pos > file_size - 4096 )
  {
    cout << "Offset is not at the entry of a block" << endl;
    exit(1);
  }

  int capacity;
  int used;
  int usage;

  data.seekg(pos);                                  // go to the meta-data
  data.read((char *)&capacity, sizeof(int));
  data.read((char *)&used, sizeof(int));
  data.read((char *)&usage, sizeof(int));

  used++;
  data.seekp(pos + sizeof(int));                    // go to the used
  data.write((char *)&used, sizeof(int));           // update used

  int i;                                            // look for vaild slot
  for ( i = 0; i < capacity; i++ )
    if ( !(usage >> i & 1) )
      break;

  usage |= 1u << i;                                 // updata usage
  data.write((char *)&usage, sizeof(int));

  int write_pos = pos + metaSize + max_record_len * i;
  data.seekp(write_pos);                            // go to the next empty slot
  data.write((char *)&emp_buffer, max_record_len);

  data.close();
  return write_pos;
}
