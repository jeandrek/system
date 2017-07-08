/*
 * Mini system package manager
 *
 * Copyright (C) 2017 Jeandre Kruger
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

void follow_command(FILE *, const char *[], int);
void operate_on_all_packages(FILE *, const char *);
void operate_on_packages_named(FILE *, const char *, const char *[], int);
void operate_on_next_package(FILE *, const char *);
void operate_on_package(FILE *, const char *, const char *);

void print_log(const char *, ...);

int strin(const char *, const char *[], int);
char fpeek(FILE *);

int main(int argc, const char *argv[])
{
  struct utsname uname_data;
  char file_path[128];
  FILE *file;

  if (argc < 2)
  {
    print_log("No command!");
    return EXIT_FAILURE;
  }

  uname(&uname_data);
  setenv("PACKAGE_DIRECTORY", "/etc/package", 0);
  setenv("TARGET", uname_data.machine, 0);
  setenv("ROOT", "", 0);

  snprintf(file_path, 128, "%s/PACKAGES", getenv("PACKAGE_DIRECTORY"));
  file = fopen(file_path, "r");
  follow_command(file, argv + 1, argc - 1);
  fclose(file);

  return EXIT_SUCCESS;
}

void follow_command(FILE *file, const char *command[], int command_length)
{
  if (!strcmp(command[0], "help"))
  {
    print_log("Commands:");
    print_log("");
    print_log("help               Show this help.");
    print_log("build package...   Build each package listed, or build every package if none listed.");
    print_log("install package... Install each package listed, or install every package if none listed.");
  }
  else if (!strcmp(command[0], "build") || !strcmp(command[0], "install"))
  {
    if (command_length == 1)
      operate_on_all_packages(file, command[0]);
    else
      operate_on_packages_named(file, command[0], command + 1, command_length - 1);
  }
  else
  {
    print_log("Unknown command: %s", command[0]);
    exit(EXIT_FAILURE);
  }
}

void operate_on_all_packages(FILE *file, const char *operation)
{
  print_log("Compiling all packages...");
  while (!feof(file))
    operate_on_next_package(file, operation);
  print_log("Done compiling all packages!");
}

void operate_on_packages_named(FILE *file, const char *operation, const char *names[], int names_count)
{
  char line[32];
  while (!feof(file))
  {
    fgets(line, 32, file);
    line[strlen(line) - 1] = '\0';
    if (line[0] != ' ' && strin(line, names, names_count))
      operate_on_package(file, operation, line);
  }
}

void operate_on_next_package(FILE *file, const char *operation)
{
  char package_name[32];

  fgets(package_name, 32, file);
  package_name[strlen(package_name) - 1] = '\0';

  operate_on_package(file, operation, package_name);
}

void operate_on_package(FILE *file, const char *operation, const char *package_name)
{
  char line[128];
  FILE *pipe;

  print_log("Operating on %s...", package_name);

  pipe = popen("/bin/sh", "w");
  fputs(". package/package.sh\n", pipe);
  fputs("PACKAGE=\"", pipe);
  fputs(package_name, pipe);
  fputs("\"\n", pipe);

  do {
    fgets(line, 128, file);
    fputs(line + 2, pipe);
  } while (!feof(file) && fpeek(file) == ' ');

  fputs(operation, pipe);
  fputs("_package\n", pipe);
  pclose(pipe);
  fgetc(file); /* blank line */

  print_log("Done with %s!", package_name);
}

void print_log(const char *format, ...)
{
  va_list args;
  va_start (args, format);
  printf("\033[1;33mPACKAGE:\033[0m ");
  vprintf(format, args);
  putchar('\n');
  va_end(args);
}


int strin(const char *str, const char *strarr[], int strcount)
{
  for (int index = 0; index < strcount; index++)
    if (!strcmp(str, strarr[index])) return 1;
  return 0;
}

char fpeek(FILE *file)
{
  char c = fgetc(file);
  ungetc(c, file);
  return c;
}
