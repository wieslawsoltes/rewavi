#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>

#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT 3
#endif

#include "version.h"

#pragma comment(lib, "vfw32.lib")

#define SUBFORMAT_GUID(fmt) \
    {fmt, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}}

#define DEFAULT_MASK_COUNT 9
#define DEFAULT_MASK {0, 4, 3, 259, 51, 55, 63, 319, 255}
#define BUFFSIZE 0x20000 /* 128KiB */

typedef enum {
    FORMAT_WAV,
    FORMAT_RAW
} format_t;

static void usage(void);
static void usage2(void);
static uint32_t numofbits(uint32_t bits);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        return 1;
    }

    if (!strcmp(argv[1], "-h"))
    {
        usage2();
        return 1;
    }

    if (!argv[2] || !_stricmp(argv[2], "-x") || !_stricmp(argv[2], "-r"))
    {
        fprintf(stderr, "output target is not specified.\n");
        return 2;
    }

    char* input = argv[1];
    char* output = argv[2];

    format_t format = FORMAT_WAV;
    if (argc == 4)
    {
        if (!_stricmp(argv[3], "-r"))
        {
            format = FORMAT_RAW;
        }
        else if (!_stricmp(argv[3], "-x"))
        {
            fprintf(stderr, "channel mask is not specified.\n");
            return 2;
        }
        else
        {
            fprintf(stderr, "raw format is not specified.\n");
            return 2;
        }
    }

    uint32_t chmask = 0;
    if (argc == 5)
    {
        if (!_stricmp(argv[3], "-x"))
        {
            chmask = atoi(argv[4]);
        }
        else
        {
            fprintf(stderr, "channel mask is not specified.\n");
            return 2;
        }
    }

    AVIFileInit();
    FILE* output_fh = NULL;
    PAVIFILE avifile = NULL;
    PAVISTREAM avistream = NULL;
    AVISTREAMINFO stream_info;
    WAVEFORMATEX wavefmt;
    LONG fmtsize = sizeof(WAVEFORMATEX);
    int id = 0;

    if (AVIFileOpen(&avifile, input, OF_READ | OF_SHARE_DENY_WRITE, NULL))
    {
        fprintf(stderr, "Could not open AVI file \"%s\".\n", input);
        AVIFileExit();
        return 1;
    }

    while (1)
    {
        if (AVIFileGetStream(avifile, &avistream, 0, id))
            break;

        if (AVIStreamInfo(avistream, &stream_info, sizeof(AVISTREAMINFO)))
            goto release_stream;

        if (stream_info.fccType != streamtypeAUDIO)
            goto release_stream;

        fprintf(stderr, "Found an audio track(ID %d) in %s\n", id, input);
        if (AVIStreamReadFormat(avistream, 0, &wavefmt, &fmtsize))
            goto release_stream;

        if (wavefmt.wFormatTag == WAVE_FORMAT_PCM ||
            wavefmt.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            break;

    release_stream:
        AVIStreamRelease(avistream);
        avistream = NULL;
        id++;
    }

    if (!avistream)
    {
        fprintf(stderr, "Could not find PCM audio track in \"%s\".\n", input);
        AVIFileRelease(avifile);
        AVIFileExit();
        return 1;
    }

    fprintf(stderr, "streamID %d: %uchannels, %uHz, %ubits, %.3fseconds.\n",
        id, wavefmt.nChannels, wavefmt.nSamplesPerSec, wavefmt.wBitsPerSample,
        1.0 * stream_info.dwLength * stream_info.dwScale / stream_info.dwRate);

    if (wavefmt.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        fprintf(stderr, "Audio track contains floating-point samples.\n");

    if (BUFFSIZE < wavefmt.nBlockAlign)
    {
        fprintf(stderr, "Too large samples; audio track is definitely invalid.\n");
        AVIStreamRelease(avistream);
        AVIFileRelease(avifile);
        AVIFileExit();
        return 1;
    }

    if (chmask)
    {
        if (numofbits(chmask) != wavefmt.nChannels)
        {
            fprintf(stderr, "Invalid channel mask was specified.\n");
            AVIStreamRelease(avistream);
            AVIFileRelease(avifile);
            AVIFileExit();
            return 1;
        }
    }
    else
    {
        uint32_t default_chmask[DEFAULT_MASK_COUNT] = DEFAULT_MASK;
        chmask = wavefmt.nChannels < DEFAULT_MASK_COUNT ?
            default_chmask[wavefmt.nChannels] : (1 << wavefmt.nChannels) - 1;
    }

    int dupout = 0;
    if (!strcmp(output, "-"))
    {
        dupout = _dup(_fileno(stdout));
        fclose(stdout);
        _setmode(dupout, _O_BINARY);
        output_fh = _fdopen(dupout, "wb");
    }
    else
    {
        fopen_s(&output_fh, output, "wb");
    }

    if (!output_fh)
    {
        fprintf(stderr, "Fail to create/open file.\n");
        AVIStreamRelease(avistream);
        AVIFileRelease(avifile);
        AVIFileExit();
        return 1;
    }

    char buffer[BUFFSIZE];
    uint32_t samples_in_buffer = BUFFSIZE / wavefmt.nBlockAlign;
    LONG samples_read;
    DWORD nextsample = 0;
    int nProgress = 0;
    int nPreviousProgress = -1;

    if (format == FORMAT_WAV)
    {
        uint32_t headersize = chmask ? 60 : 36;
        uint64_t filesize = (uint64_t)wavefmt.nBlockAlign * stream_info.dwLength;
        if (filesize > 0xFFFFFFFF - headersize)
        {
            fprintf(stderr, "WAV file will be larger than 4GB.\n");
            filesize = wavefmt.nBlockAlign * (0xFFFFFFFF - headersize / wavefmt.nBlockAlign);
        }
        uint32_t size_for_write = (uint32_t)filesize + headersize;

        /* riff file header */
        fwrite("RIFF", 1, 4, output_fh);
        fwrite(&size_for_write, 4, 1, output_fh);
        fwrite("WAVE", 1, 4, output_fh);

        /* format chunk */
        fwrite("fmt ", 1, 4, output_fh);
        size_for_write = chmask ? 40 : 16;
        fwrite(&size_for_write, 4, 1, output_fh);

        /* As for the WAVEFORMATEX / WAVEFORMATEXTENSIBLE structure, alignment is taken into consideration. */
        if (!chmask)
        {
            /* never write cbSize! */
            fwrite(&wavefmt, fmtsize - 2, 1, output_fh);
        }
        else
        {
            WAVEFORMATEXTENSIBLE wavefmt_ext = {
                wavefmt, {wavefmt.wBitsPerSample}, chmask,
                SUBFORMAT_GUID(wavefmt.wFormatTag) };
            wavefmt_ext.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            wavefmt_ext.Format.cbSize = 22;
            fwrite(&wavefmt_ext, sizeof(WAVEFORMATEXTENSIBLE), 1, output_fh);
        }

        /* data chunk*/
        fwrite("data", 1, 4, output_fh);
        size_for_write = (uint32_t)filesize;
        fwrite(&size_for_write, 4, 1, output_fh);

        fflush(output_fh);
    }

    fprintf(stderr, "Writing %s file %s ...\n", (format == FORMAT_WAV) ? "WAV" : "RAW", dupout ? "to stdout" : output);

    /* fraction processing at first. */
    AVIStreamRead(avistream, 0, stream_info.dwLength % samples_in_buffer, &buffer, BUFFSIZE, NULL, &samples_read);
    while (nextsample < stream_info.dwLength)
    {
        fwrite(buffer, wavefmt.nBlockAlign, samples_read, output_fh);
        nextsample += samples_read;

        nProgress = (int)(((double)nextsample / (double)stream_info.dwLength) * 100);
        if (nProgress != nPreviousProgress)
        {
            fprintf(stderr, "\rProgress: %d%% (%lu/%lu)", nProgress, nextsample, stream_info.dwLength);
            nPreviousProgress = nProgress;
        }

        /* The last reading is just ignored. */
        AVIStreamRead(avistream, nextsample, samples_in_buffer, &buffer, BUFFSIZE, NULL, &samples_read);
    }

    fprintf(stderr, "\n");
    fflush(output_fh);

    fclose(output_fh);
    AVIStreamRelease(avistream);
    AVIFileRelease(avifile);
    AVIFileExit();

    if (nextsample != stream_info.dwLength)
    {
        fprintf(stderr, "Writing failed.\n");
        return 1;
    }
    else
    {
        fprintf(stderr, "File written successfully.\n");
        return 0;
    }
}

static void usage(void)
{
    fprintf(stderr,
        "REWAVI - rewritten/modified WAVI ver.%s\n"
        " (c) 2011 Oka Motofumi\n"
        " (c) 2018 Wieslaw Soltes\n"
        " WAVI original - (c) 2007 Tamas Kurucsai, with lots of help from tebasuna51\n"
        "Licensed under the terms of the GNU General Public License version2 or later.\n"
        "\n"
        "Usage: rewavi <input> <output> [ -r | -x [<mask>] ]\n"
        "  input  : input AVI1.0( or AviSynth Script)file name.\n"
        "  output : output WAV file name. '-' means output to stdout\n"
        "  -r : Write a raw file of samples without the WAV header.\n"
        "  -x : Write an extended WAV header containing channel mask for\n"
        "       multi-channel audio.\n"
        "       If channel mask is not specified, the defaut value will be set.\n"
        "\n"
        " 'rewavi -h' will display more complicated description.\n", VERSION);
}

static void usage2(void)
{
    usage();
    fprintf(stderr,
        "\n"
        "This utility extracts the first uncompressed PCM audio track\n"
        "from an AVI1.0 file and saves it to a WAV file. This is not quite\n"
        "useful for most AVI files since they usually contain some kind\n"
        "of compressed audio, but it can come handy when it's needed to\n"
        "save an audio track from AviSynth.\n"
        "\n"
        "If <input> is a valid AVI1.0 file which contains PCM audio,\n"
        "the audio track will be written to <output> as a WAV(or RAW) file.\n"
        "If '-' is passed as <output>, the WAV file will be written\n"
        "to the standard output.\n"
        "If <output> is not given, only information will be printed\n"
        "about the audio track.\n"
        "\n"
        "On success, the exit code will be 0 and the first line printed\n"
        "to the standard error will look like the following:\n"
        "    streamID <n>: <c>channels, <r>Hz, <b>bits, <l>sec.\n"
        "\n"
        "Where <n> is IDnumber of audio track that comtaining PCM audio, <c> is\n"
        "the number of audio channels, <r> is the sampling rate, <b> is the number\n"
        "of bits per sample and <l> is the length of the track in seconds.\n"
        "\n"
        "If the audio track contains floating-point samples, the next line\n"
        "printed to the standard error will be:\n"
        "Audio track contains floating-point samples.\n"
        "\n"
        "If an error occurs, the exit code will be 1 and some useful error\n"
        "message will be printed to the standard error.\n"
        "\n"
        "WAV files larger than 4 GB may be created. However, such WAV files\n"
        "are non-standard and may not be handled correctly by some players\n"
        "and encoders. A warning will be printed to the standard error when\n"
        "such a WAV file is created.\n"
        "\n"
        "The default channel masks are:\n"
        "\n"
        "Mask  Ch.  MS channels                   Description\n"
        "----  ---  ----------------------------  ----------------\n"
        "   4   1                       FC        Mono\n"
        "   3   2                          FR FL  Stereo\n"
        " 259   3   BC                     FR FL  First Surround\n"
        "  51   4              BR BL       FR FL  Quadro\n"
        "  55   5              BR BL    FC FR FL  like Dpl II (without LFE)\n"
        "  63   6              BR BL LF FC FR FL  Standard Surround\n"
        " 319   7   BC         BR BL LF FC FR FL  With back center\n"
        " 255   8      FLC FRC BR BL LF FC FR FL  With front center left/right\n"
        "\n"
        "Some other common channel masks:\n"
        "\n"
        "Mask  Ch. MS channels                    Description\n"
        "----  --- -----------------------------  ----------------\n"
        "   7   3                       FC FR FL\n"
        " 263   4  BC                   FC FR FL  like Dpl I\n"
        " 271   5  BC                LF FC FR FL\n"
        "  59   5              BR BL LF    FR FL\n");
}

static uint32_t numofbits(uint32_t bits)
{
    bits = (bits & 0x55555555) + (bits >> 1 & 0x55555555);
    bits = (bits & 0x33333333) + (bits >> 2 & 0x33333333);
    bits = (bits & 0x0f0f0f0f) + (bits >> 4 & 0x0f0f0f0f);
    bits = (bits & 0x00ff00ff) + (bits >> 8 & 0x00ff00ff);
    return (bits & 0x0000ffff) + (bits >> 16 & 0x0000ffff);
}
