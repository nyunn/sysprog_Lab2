//--------------------------------------------------------------------------------------------------
// System Programming                         I/O Lab                                   Spring 2024
//
/// @file
/// @brief resursively traverse directory tree and list all entries
/// @author <jungyunOh>
/// @studid <2022-19510>
//--------------------------------------------------------------------------------------------------

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <grp.h>
#include <pwd.h>

#define MAX_DIR 64            ///< maximum number of supported directories

/// @brief output control flags
#define F_DIRONLY   0x1       ///< turn on direcetory only option
#define F_SUMMARY   0x2       ///< enable summary
#define F_VERBOSE   0x4       ///< turn on verbose mode

/// @brief struct holding the summary
struct summary {
  unsigned int dirs;          ///< number of directories encountered
  unsigned int files;         ///< number of files
  unsigned int links;         ///< number of links
  unsigned int fifos;         ///< number of pipes
  unsigned int socks;         ///< number of sockets

  unsigned long long size;    ///< total size (in bytes)
};


/// @brief abort the program with EXIT_FAILURE and an optional error message
///
/// @param msg optional error message or NULL
void panic(const char *msg)
{
  if (msg) fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}


/// @brief read next directory entry from open directory 'dir'. Ignores '.' and '..' entries
///
/// @param dir open DIR* stream
/// @retval entry on success
/// @retval NULL on error or if there are no more entries
struct dirent *getNext(DIR *dir)
{
  struct dirent *next;
  int ignore;

  do {
    errno = 0;
    next = readdir(dir);
    if (errno != 0) perror(NULL);
    ignore = next && ((strcmp(next->d_name, ".") == 0) || (strcmp(next->d_name, "..") == 0));
  } while (next && ignore);

  return next;
}


/// @brief qsort comparator to sort directory entries. Sorted by name, directories first.
///
/// @param a pointer to first entry
/// @param b pointer to second entry
/// @retval -1 if a<b
/// @retval 0  if a==b
/// @retval 1  if a>b
static int dirent_compare(const void *a, const void *b)
{
  struct dirent *e1 = (struct dirent*)a;
  struct dirent *e2 = (struct dirent*)b;

  // if one of the entries is a directory, it comes first
  if (e1->d_type != e2->d_type) {
    if (e1->d_type == DT_DIR) return -1;
    if (e2->d_type == DT_DIR) return 1;
  }

  // otherwise sorty by name
  return strcmp(e1->d_name, e2->d_name);
}


/// @brief recursively process directory @a dn and print its tree
///
/// @param dn absolute or relative path string
/// @param depth depth in directory tree
/// @param stats pointer to statistics
/// @param flags output control flags (F_*)
void processDir(const char *dn, unsigned int depth, struct summary *stats, unsigned int flags)
{
  // TODO

  printf("%s", dn);
  DIR *dir = opendir(dn);
  if (dir == NULL) {
    printf("ERROR: Permission denied\n");
    return;
  }

  struct dirent *e;
  struct dirent **directories = malloc(MAX_DIR*sizeof(struct dirent*));

  int index=0;
  while ((e = readdir(dir)) != NULL && index < MAX_DIR) {
    //ignore special character
    if (strcmp(".", e->d_name) == 0 || strcmp("..", e->d_name) == 0) continue;
    //save the directory entry
    directories[index++] = e;
  }

  //print if summary mode
  if ((flags & F_SUMMARY) && (depth == 0)) {
    printf("Name                                                        User:Group           Size     Perms Type\n");
    printf("----------------------------------------------------------------------------------------------------\n");
  }

  //sort
  qsort(directories, index, sizeof(struct dirent *), dirent_compare);
  //print if dironly mode
  if (flags & F_DIRONLY) {
    for (int j=0; j<index; j++) {
      struct stat sb;
      char full_path[256];
      strcpy(full_path, dn);
      strcat(full_path, "/");
      strcat(full_path, directories[j]->d_name);
      if (lstat(full_path, &sb) == -1) {
          printf("ERROR: Permission denied\n");
          continue;
      }
      if (S_ISDIR(sb.st_mode)) printf("%s", directories[j]->d_name);
      else continue;
      //print if verbose mode
      if (flags & F_VERBOSE) {
          struct passwd *userInfo = getpwuid(sb.st_uid);
          struct group *groupInfo = getgrgid(sb.st_gid);

          if (userInfo == NULL) {
            printf("Permission denied.\n"); //error handling
            continue;
          }
          if (groupInfo == NULL) {
            printf("Permission denied.\n"); //error handling
            continue;
          }

          printf("%54s:%s", userInfo->pw_name, groupInfo->gr_name);

          //print the size
          printf("%5ld", sb.st_size);

          //print the permissions (read, write, excute)
          //user
          if (sb.st_mode & S_IRUSR) { //read
            printf(" r");
          } else printf(" -");
          if (sb.st_mode & S_IWUSR) { //write
            printf("w");
          } else printf("-");
          if (sb.st_mode & S_IXUSR) { //execute
            printf("x");
          } else printf("-");
          //group
          if (sb.st_mode & S_IRGRP) { //read
            printf("r");
          } else printf("-");
          if (sb.st_mode & S_IWGRP) { //write
            printf("w");
          } else printf("-");
          if (sb.st_mode & S_IXGRP) { //execute
            printf("x");
          } else printf("-");
          //others
          if (sb.st_mode & S_IROTH) { //read
            printf("r");
          } else printf("-");
          if (sb.st_mode & S_IWOTH) { //write
            printf("w");
          } else printf("-");
          if (sb.st_mode & S_IXOTH) { //execute
            printf("x");
          } else printf("-");

          //print the type
          printf(" d");
      }
      printf("\n");
      //print recursively
      DIR *dir_tmp;
      dir_tmp = opendir(full_path);
      //struct dirent **subdirectories = malloc(MAX_DIR * sizeof(struct diretnt*));
      while (getNext(dir_tmp) != NULL) {
        printf("  ");
        processDir(full_path, depth + 1, stats, flags);
      }
      closedir(dir_tmp);
    }
  } else { //except dironly mode
    for (int j=0; j<index; j++) {
      struct stat sb;
      char full_path[256];
      strcpy(full_path, dn);
      strcat(full_path, "/");
      strcat(full_path, directories[j]->d_name);
      if (lstat(full_path, &sb) == -1) {
          printf("ERROR: Permission denied\n");
          continue;
      }

      printf("%s", directories[j]->d_name);
      //print if verbose mode
      if (flags & F_VERBOSE) {
          struct passwd *userInfo = getpwuid(sb.st_uid);
          struct group *groupInfo = getgrgid(sb.st_gid);

          if (userInfo == NULL) {
            printf("Permission denied.\n"); //error handling
            continue;
          }
          if (groupInfo == NULL) {
            printf("Permission denied.\n"); //error handling
            continue;
          }

          printf("%54s:%s", userInfo->pw_name, groupInfo->gr_name);

          //print the size
          printf("%5ld", sb.st_size);

          //print the permissions (read, write, excute)
          //user
          if (sb.st_mode & S_IRUSR) { //read
            printf(" r");
          } else printf(" -");
          if (sb.st_mode & S_IWUSR) { //write
            printf("w");
          } else printf("-");
          if (sb.st_mode & S_IXUSR) { //execute
            printf("x");
          } else printf("-");
          //group
          if (sb.st_mode & S_IRGRP) { //read
            printf("r");
          } else printf("-");
          if (sb.st_mode & S_IWGRP) { //write
            printf("w");
          } else printf("-");
          if (sb.st_mode & S_IXGRP) { //execute
            printf("x");
          } else printf("-");
          //others
          if (sb.st_mode & S_IROTH) { //read
            printf("r");
          } else printf("-");
          if (sb.st_mode & S_IWOTH) { //write
            printf("w");
          } else printf("-");
          if (sb.st_mode & S_IXOTH) { //execute
            printf("x");
          } else printf("-");

          //print the type
          if (S_ISDIR(sb.st_mode)) {
            printf("  d");
            stats->dirs++;
          } else if (S_ISLNK(sb.st_mode)) {
            printf("  l");
            stats->links++;
          } else if (S_ISCHR(sb.st_mode)) {
            printf("  c");
          } else if (S_ISBLK(sb.st_mode)) {
            printf("  b");
          } else if (S_ISFIFO(sb.st_mode)) {
            printf("  f");
            stats->fifos++;
          } else if (S_ISSOCK(sb.st_mode)) {
            printf("  s");
            stats->socks++;
          }
      }
      printf("\n");
      //print recursively
      if (S_ISDIR(sb.st_mode)) {

        DIR *dir_tmp;
        dir_tmp = opendir(full_path);
        while (getNext(dir_tmp) != NULL) {
          printf("  ");
          processDir(full_path, depth + 1, stats, flags);
        }
        closedir(dir_tmp);
      }
    }
  }

  if ((flags & F_SUMMARY) && (depth == 0)) {
    printf("----------------------------------------------------------------------------------------------------\n");
    if (flags & F_DIRONLY) {
      if (stats->dirs == 1) printf("1 directory\n");
      else printf("%d directories\n", stats->dirs);
    } else {
      if (stats->files == 1) printf("1 file, ");
      else printf("%d files, ", stats->files);

      if (stats->dirs == 1) printf("1 directory, ");
      else printf("%d directories, ", stats->dirs);

      if (stats->links == 1) printf("1 link, ");
      else printf("%d links, ", stats->links);

      if (stats->fifos == 1) printf("1 pipe, ");
      else printf("%d pipes, ", stats->fifos);

      if (stats->socks == 1) printf("and 1 socket\n");
      else printf("and %d sockets\n", stats->socks);
    }
  }

  closedir(dir);
}

/// @brief print program syntax and an optional error message. Aborts the program with EXIT_FAILURE
///
/// @param argv0 command line argument 0 (executable)
/// @param error optional error (format) string (printf format) or NULL
/// @param ... parameter to the error format string
void syntax(const char *argv0, const char *error, ...)
{
  if (error) {
    va_list ap;

    va_start(ap, error);
    vfprintf(stderr, error, ap);
    va_end(ap);

    printf("\n\n");
  }

  assert(argv0 != NULL);

  fprintf(stderr, "Usage %s [-d] [-s] [-v] [-h] [path...]\n"
                  "Gather information about directory trees. If no path is given, the current directory\n"
                  "is analyzed.\n"
                  "\n"
                  "Options:\n"
                  " -d        print directories only\n"
                  " -s        print summary of directories (total number of files, total file size, etc)\n"
                  " -v        print detailed information for each file. Turns on tree view.\n"
                  " -h        print this help\n"
                  " path...   list of space-separated paths (max %d). Default is the current directory.\n",
                  basename(argv0), MAX_DIR);

  exit(EXIT_FAILURE);
}


/// @brief program entry point
int main(int argc, char *argv[])
{
  //
  // default directory is the current directory (".")
  //
  const char CURDIR[] = ".";
  const char *directories[MAX_DIR];
  int   ndir = 0;

  struct summary tstat;
  unsigned int flags = 0;

  //
  // parse arguments
  //
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      // format: "-<flag>"
      if      (!strcmp(argv[i], "-d")) flags |= F_DIRONLY;
      else if (!strcmp(argv[i], "-s")) flags |= F_SUMMARY;
      else if (!strcmp(argv[i], "-v")) flags |= F_VERBOSE;
      else if (!strcmp(argv[i], "-h")) syntax(argv[0], NULL);
      else syntax(argv[0], "Unrecognized option '%s'.", argv[i]);
    } else {
      // anything else is recognized as a directory
      if (ndir < MAX_DIR) {
        directories[ndir++] = argv[i];
      } else {
        printf("Warning: maximum number of directories exceeded, ignoring '%s'.\n", argv[i]);
      }
    }
  }

  // if no directory was specified, use the current directory
  if (ndir == 0) directories[ndir++] = CURDIR;


  //
  // process each directory
  //
  // TODO
  //
  // Pseudo-code
  // - reset statistics (tstat)
  // - loop over all entries in 'directories' (number of entires stored in 'ndir')
  //   - reset statistics (dstat)
  //   - if F_SUMMARY flag set: print header
  //   - print directory name
  //   - call processDir() for the directory
  //   - if F_SUMMARY flag set: print summary & update statistics
  memset(&tstat, 0, sizeof(tstat));
  for (int i=0; i<ndir; i++) {
    struct summary stats;
    memset(&stats, 0, sizeof(stats));
    processDir(directories[i], 0, &stats, flags);
    tstat.files += stats.files;
    tstat.dirs += stats.dirs;
    tstat.fifos += stats.fifos;
    tstat.links += stats.links;
    tstat.socks += stats.socks;
    tstat.size += stats.size;
  }


  //
  // print grand total
  //
  if ((flags & F_SUMMARY) && (ndir > 1)) {
    if (flags & F_DIRONLY) {
      printf("Analyzed %d directories:\n"
             "  total # of directories:  %16d\n",
             ndir, tstat.dirs);
    } else {
      printf("Analyzed %d directories:\n"
             "  total # of files:        %16d\n"
             "  total # of directories:  %16d\n"
             "  total # of links:        %16d\n"
             "  total # of pipes:        %16d\n"
             "  total # of sockets:      %16d\n",
             ndir, tstat.files, tstat.dirs, tstat.links, tstat.fifos, tstat.socks);

      if (flags & F_VERBOSE) {
        printf("  total file size:         %16llu\n", tstat.size);
      }
    }
  }

  //
  // that's all, folks!
  //
  return EXIT_SUCCESS;
}
