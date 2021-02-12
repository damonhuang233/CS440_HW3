#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <stdlib.h>

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
  int usage;
  int overflow;
  char data[4080];
};

int         mode        = 0;          // 0 for Index creation mode / 1 for Look up mode
int         metaSize    = 16;         // number of bytes for the meta data of a block

struct Emp  buffer;                   // Emp buffer for reading file
string      linebuffer;               // line buffer for reading a single line from file

fstream     csv;                      // file pointer to read csv file
fstream     idx;                      // file pointer for EmployeeIndex file
fstream     data;                     // file pointer for EmployeeData file

int         read_line();                      // read a line from the csv file and put data in to the Emp struct
int         hash_id( char *id );              // hash the id and return the key
void        init_index();                     // init hash index
void        init_data();                      // init block
int         write_cur_record( int pos );      // write

int main(int argc, char **argv)
{

  if ( argc != 2 || (strcmp(argv[1], "-C") != 0 && strcmp(argv[1], "-L")) )
  {
    cout << "Usage: main.out [-C/-L]     C: Index creation mode / L: Lookup mode" << endl;
    exit(0);
  }

  if ( strcmp(argv[1], "C") != 0 )
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
      int key = hash_id(buffer.id);
    }
    csv.close();
  }

  return 0;
}

int read_line()
{
  if (getline(csv, linebuffer))
  {
    int i = 0;
    int c = 0;
    for ( i = 0; i < 8; i++ )                      // read id
    {
      buffer.id[i] = linebuffer[c];
      c++;
    }

    c++;                                           //  skip ,

    for ( i = 0; linebuffer[c] != ','; i++)        //  read name
    {
      buffer.name[i] = linebuffer[c];
      c++;
    }
    buffer.name[i] = '\0';                         //  end of name

    c++;                                           //  skip ,

    for ( i = 0; linebuffer[c] != ','; i++)        //  read name
    {
      buffer.bio[i] = linebuffer[c];
      c++;
    }
    buffer.bio[i] = '\0';                          //  end of bio

    c++;                                           //  skip ,

    for ( i = 0; i < 8; i++ )                      // read manager_id
    {
      buffer.manager_id[i] = linebuffer[c];
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
  new_block.used = 4;
  new_block.usage = 0;
  new_block.overflow = 0;

  data.write((char *) &new_block, sizeof(struct Block));

  data.close();
}

int write_cur_record ( int pos )
{
  data.open("EmployeeData", ios::out | ios::binary);

  if ( !data.is_open() )
  {
    cout << "Failed to open EmployeeData file" << endl;
    exit(1);
  }

  // In progress.........
  // ref https://courses.cs.vt.edu/cs2604/fall02/binio.html

  data.close();
  return 0;
}
