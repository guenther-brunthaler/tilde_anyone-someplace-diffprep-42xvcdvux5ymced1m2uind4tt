#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <locale.h>
#include <limits.h>


#if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901
   #include <wctype.h>
#else
   #include <ctype.h>
   #ifdef iswspace
      #undef iswspace
   #endif
   #define iswspace(wc) ( \
      (wc) <= (wchar_t)UCHAR_MAX && isspace((int)(unsigned char)(wc)) \
   )
#endif


#define CMD_WORD ':'
#define CMD_NEWLINE 'n'
#define NL_RPLC ' '

static void die(char const *msg, ...) {
   va_list args;
   va_start(args, msg);
   (void)vfprintf(stderr, msg, args);
   va_end(args);
   (void)fputc('\n', stderr);
   exit(EXIT_FAILURE);
}

static void chk_stdin(void) {
   if (ferror(stdin)) die("Error reading from standard input stream!");
}

static void stdout_error(void) {
   die("Error writing to standard output stream!");
}

static void chk_stdout(void) {
   if (ferror(stdout)) stdout_error();
}

static void ck_printf(char const *format, ...) {
   va_list args;
   va_start(args, format);
   if (vprintf(format, args) < 0) stdout_error();
   va_end(args);
}

static void ck_putc(int c) {
   if (putchar(c) != c) stdout_error();
}

static void ck_write(char *buf, size_t bytes) {
   if (fwrite(buf, sizeof(char), bytes, stdout) != bytes) stdout_error();
}

static void perror_exit(int e, char const *msg) {
   errno= e;
   perror(msg);
   exit(EXIT_FAILURE);
}

static void die_with_wchar(char const *msg, wchar_t wc) {
   char chr[MB_LEN_MAX + 1];
   int r;
   if ((r= wctomb(chr, wc)) < 1) {
      die(
            "Cannot display error message \"%s\" for wide character"
            " with code point %#lx!"
         ,  msg, (unsigned long)wc
      );
   }
   assert((size_t)r < sizeof chr);
   chr[r]= '\0';
   die(msg, chr);
}

int main(int argc, char **argv) {
   int mode= 'w';
   (void)setlocale(LC_ALL, "");
   if (argc > 1) {
      int optind, argpos;
      char *arg= argv[optind= 1];
      for (argpos= 0;; ) {
         int c;
         switch (c= arg[argpos]) {
            case '-':
               if (argpos) goto bad_option;
               switch (arg[++argpos]) {
                  case '-':
                     if (c= arg[argpos + 1]) {
                        die("Unsupported long option %s!", arg);
                     }
                     ++optind; /* "--". */
                     goto end_of_options;
                  case '\0': goto end_of_options; /* "-". */
               }
               /* At first option switch character now. */
               continue;
            case '\0': {
               if (++optind == argc) goto end_of_options;
               arg= argv[optind];
               argpos= 0;
               continue;
            }
         }
         if (!argpos) goto end_of_options;
         switch (c) {
            case 'W': case 'C': case 'X': case 'B':
            case 'w': case 'c': case 'x': case 'b':
               mode= c;
               break;
            default: bad_option: die("Unsupported option -%c!", c);
         }
         ++argpos;
      }
      end_of_options:
      if (optind < argc) {
         char const *fname= argv[optind++];
         char const *fmode= strchr("xb", mode) ? "rb" : "r";
         if (!freopen(fname, fmode, stdin)) {
            die("Could not open file \"%s\" in mode \"%s\"!", fname, fmode);
         }
      }
      if (optind != argc) die("Too many arguments!");
   }
   /* Reset initial multibyte character conversion shift state - just to be
    * sure. */
   (void)mbtowc(0, 0, 0);
   switch (mode) {
      int c;
      case 'x': case 'b':
         {
            register int c;
            while ((c= getchar()) != EOF) {
               if (mode == 'b' || c == 0x20) ck_printf("%02X\n", c);
               else ck_printf("%02X %c\n", c, c > 0x20 && c < 0x7f ? c : '.');
            }
         }
         chk_stdin();
         break;
      case 'X': case 'B':
         {
            auto int c;
            int n;
            errno= 0;
            while ((n= scanf("%2x", &c)) == 1) {
               if (putchar(c) == EOF) stdout_error();
               while ((c= getchar()) != '\n') {
                  if (c == EOF) {
                     chk_stdin();
                     goto done;
                  }
               }
            }
            if (n == EOF) {
               int e= errno;
               chk_stdin();
               if (e) perror_exit(e, "Parsing error");
            }
            else bad_syntax: die("Input format syntax error!");
         }
         goto done;
   }
   {
      size_t const mb_cur_max= MB_CUR_MAX;
      int state= 0;
      char c[MB_LEN_MAX];
      size_t nc0, nc= 0;
      int nnul, b, eof= 0;
      wchar_t wc;
      {
         /* Determine the length of the MBCS-encoding of L'\0'. */
         char nul[MB_LEN_MAX];
         if ((nnul= wctomb(nul, L'\0')) < 1) die("Unsupported locale!");
      }
      for (;;) {
         /* Read as much bytes into c[] as possible, but not more than the
          * longest possible MBCS-sequence. */
         while (!eof && nc < mb_cur_max) {
            if ((b= getchar()) == EOF) {
               chk_stdin();
               eof= 1;
               break;
            }
            c[nc++]= (char)b;
         }
         if (nc == 0) {
            assert(eof);
            break;
         }
         {
            int r;
            auto wchar_t wcbuf; /* Address will be taken. */
            if ((r= mbtowc(&wcbuf, c, nc)) == -1) {
               bad_mbc:
               die(
                     eof
                  ?  "Incomplete multibyte character at end of input!"
                  :  "Illegal character encoding encountered!"
               );
            }
            if (r == 0) r= nnul;
            assert(r > 0);
            assert((size_t)r <= nc);
            /* In contrary to <wcbuf>, <wc> might be a register variable. */
            wc= wcbuf;
            nc0= (size_t)r;
         }
         switch (mode) {
            case 'w': case 'c':
               if (wc == L'\n') {
                  switch (state) {
                     case 2: case 3: ck_putc('\n'); /* Fall through. */
                     case 0: ck_putc(CMD_NEWLINE); state= 1; break;
                     default: assert(state == 1); ck_putc(NL_RPLC);
                  }
               } else if (iswspace(wc)) {
                  switch (state) {
                     case 1: ck_putc('\n'); /* Fall through. */
                     case 0: ck_putc(CMD_WORD); /* Fall through. */
                     case 3: state= 2; /* Fall through. */
                     default: assert(state == 2); ck_write(c, nc0);
                  }
               } else {
                  switch (state) {
                     case 1: case 2: ck_putc('\n'); /* Fall through. */
                     case 0: ck_putc(CMD_WORD); state= 3; /* Fall through. */
                     default: {
                        assert(state == 3); ck_write(c, nc0);
                        if (mode == 'c') {
                           ck_putc('\n');
                           state= 0;
                        }
                     }
                  }
               }
               break;
            default: {
               assert(mode == 'W' || mode == 'C');
               /* States:
                * 0: Initial state - parse command letter.
                * 1: Convert <NL_RPLC>s into newline charaters.
                * 2: Output characters before next '\n'. */
               switch (state) {
                  case 0:
                     switch (wc) {
                        case (wchar_t)CMD_WORD: state= 2; break;
                        case (wchar_t)CMD_NEWLINE: state= 1; goto nlgen;
                        default: {
                           die_with_wchar("Invalid line command \"%s\"!", wc);
                        }
                     }
                     break;
                  case 1:
                     switch (wc) {
                        case (wchar_t)NL_RPLC: nlgen: ck_putc('\n'); break;
                        case L'\n': state= 0; break;
                        default: {
                           die_with_wchar(
                              "Invalid newline substitute \"%s\"!", wc
                           );
                        }
                     }
                     break;
                  case 2: {
                     if (wc == L'\n') state= 0;
                     else ck_write(c, nc0);
                  }
               }
            }
         }
         assert(nc0 <= nc);
         if (nc0 < nc) (void)memmove(c, c + nc0, nc - nc0);
         nc-= nc0;
      }
   }
   done:
   if (fflush(0)) die("Error writing to output stream!");
   return EXIT_SUCCESS;
}
