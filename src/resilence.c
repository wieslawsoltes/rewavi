#include "common.h"

#define PRINT_LOG(level, ...) \
    rewavi_log("resilence", level, __VA_ARGS__)

#define CLOSE_IF_ERR(cond, ...) \
    if (cond) {retcode = PRINT_LOG(LOG_ERROR, __VA_ARGS__); goto close;}

#define DEFAULT_LENGTH 1.0
#define DEFAULT_CHANNELS 2
#define DEFAULT_SAMPLERATE 48000
#define DEFAULT_SAMPLEBITS 16
#define NO_DEFAULT 0
#define BUFFSIZE 0x100000 /* 1MiB*/

static void usage(void);

int main (int argc, char **argv)
{
    if (argc < 2)
        usage();

    int retcode = 0;
    FILE *output_fh = NULL;

    CLOSE_IF_ERR(!strcmp(argv[1], "-f") || !strcmp(argv[1], "-l") ||
                 !strcmp(argv[1], "-c") || !strcmp(argv[1], "-b"),
                 "Missing output target.\n");

    WAVEFORMATEX wavefmt = { WAVE_FORMAT_PCM, DEFAULT_CHANNELS, DEFAULT_SAMPLERATE,
                             NO_DEFAULT, NO_DEFAULT, DEFAULT_SAMPLEBITS, 0 };
    double length = DEFAULT_LENGTH;
    int samplebits_given = 0;

    int i = 1;
    while (argv[++i]) {
        CLOSE_IF_ERR(argv[i][0] != '-', "Invarid argument '%s'\n", argv[i]);
        switch (argv[i][1]) {
        case 'f':
            wavefmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            wavefmt.wBitsPerSample = 32;
            break;
        case 'l':
            length = atof(argv[++i]);
            CLOSE_IF_ERR(length <= 0, "Invalid length given after -l\n");
            break;
        case 'c':
            wavefmt.nChannels = atoi(argv[++i]);
            CLOSE_IF_ERR(!wavefmt.nChannels || wavefmt.nChannels > 32,
                         "Invalid number of channels '%s' given after -c\n", argv[i]);
            break;
        case 's':
            wavefmt.nSamplesPerSec = atoi(argv[++i]);
            CLOSE_IF_ERR(!wavefmt.nSamplesPerSec || wavefmt.nSamplesPerSec > 192000,
                         "Invalid sample rate '%s' given after -s\n", argv[i]);
            break;
        case 'b':
            wavefmt.wBitsPerSample = atoi(argv[++i]);
            CLOSE_IF_ERR(!wavefmt.wBitsPerSample ||
                         wavefmt.wBitsPerSample % 8 ||
                         wavefmt.wBitsPerSample > 32,
                         "Invalid bits per sample '%s' given after -b\n", argv[i]);
            samplebits_given = 1;
            break;
        default:
            PRINT_LOG((retcode = LOG_ERROR), "Invarid argument '%s'\n", argv[i]);
            goto close;
        }
    }

    CLOSE_IF_ERR(wavefmt.wFormatTag + samplebits_given > 3,
                 "'-f' and '-b' cannot be used at the same time.\n");

    wavefmt.nBlockAlign = wavefmt.nChannels * wavefmt.wBitsPerSample >> 3;
    wavefmt.nAvgBytesPerSec = wavefmt.nBlockAlign * wavefmt.nSamplesPerSec;
    uint64_t filesize = (uint64_t)(wavefmt.nAvgBytesPerSec * length);
    uint32_t size_for_write = (uint32_t)filesize + 36;
    if (filesize > 0xFFFFFFFF - 36) {
        PRINT_LOG(LOG_WARNING, "WAV file will be larger than 4 GB.\n");
        size_for_write = wavefmt.nBlockAlign * ((0xFFFFFFFF - 36) / wavefmt.nBlockAlign) + 36;
    }

    int dupout = 0;
    if (!strcmp(argv[1], "-")) {
        dupout = _dup(_fileno(stdout));
        fclose(stdout);
        _setmode(dupout, _O_BINARY);
        output_fh = _fdopen(dupout, "wb");
    } else
        fopen_s(&output_fh, argv[1], "wb");

    CLOSE_IF_ERR(!output_fh, "Fail to create/open file.\n");

    PRINT_LOG(LOG_INFO, "Writing silence WAV file %s ...\n", dupout ? "to stdout" : argv[1]);

    fwrite("RIFF", 1, 4, output_fh);
    fwrite(&size_for_write, 4, 1, output_fh);
    fwrite("WAVE", 1, 4, output_fh);
    fwrite("fmt ", 1, 4, output_fh);
    fwrite("\x10\x00\x00\x00", 1, 4, output_fh);
    fwrite(&wavefmt, sizeof(WAVEFORMATEX) - 2, 1, output_fh); /* never write cbSize! */
    fwrite("data", 1, 4, output_fh);
    size_for_write -= 36;
    fwrite(&size_for_write, 4, 1, output_fh);

    fflush(output_fh);

    char data[BUFFSIZE] = {0};
    uint64_t wrote = filesize % BUFFSIZE;
    fwrite(data, 1, (size_t)wrote, output_fh);
    while (wrote < filesize) {
        fwrite(data, 1, BUFFSIZE, output_fh);
        wrote += BUFFSIZE;
    }

    fflush(output_fh);

    CLOSE_IF_ERR(wrote != filesize, "Writing failed...\n");
    PRINT_LOG(LOG_INFO, "WAV file written successfully.\n");

close:
    if (output_fh)
        fclose(output_fh);
    return retcode;
}

static void usage(void)
{
    fprintf(stderr,
        "RESILENCE - rewritten/modified SILENCE ver.%s\n"
        "   (c) 2011 Oka Motofumi\n"
        "   original SILENCE (c) 2007 Tamas Kurucsai\n"
        " Licensed under the terms of the GNU General Public License verion2 or later\n"
        "\n"
        "This tool create silent WAV files.\n"
        "Usage: resilence <output> [-l <length>] [-c <channels>] [-s <sample-rate>]\n"
        "                          [-b <bits>] [-f]\n"
        "\n"
        "  output: output WAV file name. \"-\" means output to stdout.\n"
        "  -l : length of output stream. (unit:sec  default=1.0)\n"
        "  -c : number of channels of output stream. (default:2  min:1  max:32)\n"
        "  -s : sample rate of output stream. (unit:Hz  default:48000 max:192000)\n"
        "  -b : bits-per-sample of output stream. this value must be 8, 16, 24,\n"
        "       or 32. (default:16)\n"
        "  -f : use 32bit floating-point samples instead of 8/16/24/32bit integer.\n"
        "       this option cannot be used with -b at the same time.\n",
        VERSION);
    exit(1);
}
