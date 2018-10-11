#include "Arc.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#pragma pack(1)

#define CDATE_FREQ 49710 // Hz

class CDate
{
public:
  uint32_t time;
  int32_t date;
};

uint16_t EndianWORD(uint16_t d)
{
  return ((d & 255) << 8) | ((d >> 8) & 255);
}

uint32_t EndianDWORD(uint32_t d)
{
  return ((d & 255) << 24) | (((d >> 8) & 255) << 16) |
         (((d >> 16) & 255) << 8) | ((d >> 24) & 255);
}

uint32_t StrOcc(char *src, char ch)
{ //Count occurrences of a char.
  uint32_t i = 0;
  if (!src)
    return 0;
  while (*src)
    if (*src++ == ch)
      i++;
  return i;
}

void *CAlloc(uint32_t size)
{
  uint8_t *res = (uint8_t *)malloc(size);
  memset(res, 0, size);
  return res;
}

void Free(void *ptr)
{
  if (ptr)
    free(ptr);
}

int FSize(FILE *f)
{
  int res, original = ftell(f);
  fseek(f, 0, SEEK_END);
  res = ftell(f);
  fseek(f, original, SEEK_SET);
  return res;
}



bool Cvt(char *in_name, char *out_name, bool cvt_ascii)
{
  uint32_t out_size, i, j, in_size;
  CArcCompress *arc;
  uint8_t *out_buf;
  FILE *io_file;
  bool okay = false;
  if (io_file = fopen(in_name, "rb"))
  {
    in_size = FSize(io_file);
    arc = (CArcCompress *)malloc(in_size);
    fread(arc, 1, in_size, io_file);
    out_size = arc->expanded_size;
    printf("%-45s %d-->%d\r\n", in_name, (uint32_t)in_size, out_size);
    fclose(io_file);
    if (arc->compressed_size == in_size &&
        arc->compression_type && arc->compression_type <= 3)
    {
      if (out_buf = arc->ExpandBuf())
      {
        if (cvt_ascii)
        {
          j = 0;
          for (i = 0; i < out_size; i++)
            if (out_buf[i] == 31)
              out_buf[j++] = 32;
            else if (out_buf[i] != 5)
              out_buf[j++] = out_buf[i];
          out_size = j;
        }
        if (io_file = fopen(out_name, "wb"))
        {
          fwrite(out_buf, 1, out_size, io_file);
          fclose(io_file);
          okay = true;
        }
        Free(out_buf);
      }
    }
    Free(arc);
  }
  return okay;
}

#define ONA_REST INT8_MIN

double Ona2Freq(char ona)
{ //Ona to freq. Ona=60 is 440.0Hz.
  if (ona == ONA_REST)
    return 0;
  else
    return 440.0/32*pow(2.0,ona/12.0);
}

class CSndData
{
public:
  double time;
  int8_t ona;
};

class CSndWaveCtrl
{
public:
  int sample_rate;
  double phase, last_y, next_y;
};

CSndWaveCtrl *SndWaveCtrlNew(int sample_rate = 8000)
{
  CSndWaveCtrl *swc = (CSndWaveCtrl *)CAlloc(sizeof(CSndWaveCtrl));
  swc->sample_rate = sample_rate;
  return swc;
}

void SndWaveAddBuf(CSndWaveCtrl *swc, uint8_t *buf, int num_samples,
                   double _freq, double _amp = 1.0)
{
  int i, j, k;
  double a, f, amp, phase;
  if (!swc)
    return;
  if (!_freq || !_amp)
    swc->last_y = swc->phase = 0;
  else
  {
    phase = swc->phase;
    i = 0;
    if (INT32_MAX < INT32_MAX * _amp)
      amp = INT32_MAX;
    else
      amp = INT32_MAX * _amp;
    f = 2.0 / swc->sample_rate * _freq;
    while (phase < 0)
      phase += 2;
    while (phase >= 2)
      phase -= 2;
    while (i < num_samples)
    {
      if (phase >= 1.0)
        j = -amp;
      else
        j = amp;
      k = j >> 16;
      *((short *)buf + i++) += k;
      phase += f;
      while (phase >= 2)
        phase -= 2;
    }
    swc->phase = phase;
  }
}

#define SNDFILE_SAMPLE_RATE 8000
//Header for a ".SND" file
class CFileSND
{
public:
  uint32_t signature; //0x646e732e
  uint32_t offset;    //24
  uint32_t data_size;
  uint32_t coding;      //3=16bit uncompressed
  uint32_t sample_rate; //Hz
  uint32_t channels;    //1=mono
  short body[1];
};

#define SND_FILE_MAX 0x007FF000

int SndFileCreate(char *print_fmt, double normalized_vol = 1.0,
                  double reverb_delay = 0, double reverb_intensity = 0, double time_shift = 0)
{
  int i, i1, k, cnt, cnt2, level, file_num;
  double dt;
  CFileSND *s;
  CSndWaveCtrl *swc = SndWaveCtrlNew(SNDFILE_SAMPLE_RATE);
  CSndData *d, *d1;
  char st[512];
  FILE *out_file;

  //  dt=snd.record_head.last->time-snd.record_head.next->time;
  if (!dt)
    return 0;
  cnt = dt * SNDFILE_SAMPLE_RATE;
  cnt++; //Terminator

  s = (CFileSND *)CAlloc(sizeof(CFileSND) - sizeof(short) + cnt * sizeof(short));
  s->signature = 0x646e732e;
  s->offset = EndianDWORD(sizeof(CFileSND) - sizeof(short));
  s->coding = EndianDWORD(3);
  s->sample_rate = EndianDWORD(SNDFILE_SAMPLE_RATE);
  s->channels = EndianDWORD(1);

  if (time_shift)
  {
    //    d=snd.record_head.next;
    d->time -= time_shift;
    //    while (d->next!=&snd.record_head) {
    while (true)
    {
      //      d1=d->next;
      dt = d1->time - d->time;
      if (dt < 0)
      {
        //	QueRem(d1);
        //	Free(d1);
      }
      else
        break;
    }
  }

  //  d=snd.record_head.next;
  k = 0;
  i = d->time * SNDFILE_SAMPLE_RATE;
  //  while (d->next!=&snd.record_head) {
  //    d1=d->next;
  i1 = d1->time * SNDFILE_SAMPLE_RATE;
  if (i1 - i)
  {
    SndWaveAddBuf(swc, (uint8_t *)&s->body[k], i1 - i,
                  Ona2Freq(d->ona), normalized_vol);
    k += i1 - i;
    i = i1;
  }
  //    QueRem(d);
  //    Free(d);
  d = d1;
  //  }

  //Reverb
  if (reverb_intensity)
  {
    if (dt = reverb_delay * SNDFILE_SAMPLE_RATE)
      for (i = dt; i < cnt; i++)
        s->body[i] += reverb_intensity * s->body[i - (uint32_t)dt];
  }

  //Get rid of D.C. component
  for (k = 0; k < 3; k++)
  {
    level = 0;
    for (i = 0; i < cnt - 1; i++)
      level += s->body[i];
    level /= cnt - 1;
    for (i = 0; i < cnt - 1; i++)
    {
      if (s->body[i] - level <= INT16_MIN)
        s->body[i] = INT16_MIN;
      else if (s->body[i] - level >= INT16_MIN)
        s->body[i] = INT16_MAX;
      else
        s->body[i] = s->body[i] - level;
    }
  }

  for (i = 0; i < cnt - 1; i++)
    s->body[i] = EndianWORD(s->body[i]);

  s->body[cnt - 1] = 0;

  //  d=snd.record_head.next;
  //  while (d!=&snd.record_head) {
  //    d1=d->next;
  //    QueRem(d);
  //    Free(d);
  d = d1;
  //  }

  cnt2 = cnt;
  file_num = 0;
  while (cnt2 > 0)
  {
    i = cnt2;
    if (i > SND_FILE_MAX)
      i = SND_FILE_MAX;
    s->data_size = EndianDWORD(i * sizeof(short));
    memcpy(s->body, &s->body[file_num * SND_FILE_MAX], i * sizeof(short));
    sprintf(st, print_fmt, file_num++);
    if (out_file = fopen(st, "wb"))
    {
      fwrite(s, 1, sizeof(CFileSND) - sizeof(short) + i * sizeof(short), out_file);
      fclose(out_file);
    }
    cnt2 -= i;
  }
  Free(s);
  Free(swc);
  return cnt;
}

#define GR_WIDTH 640
#define GR_HEIGHT 480
#define COLORS_NUM 16

class CBGR24
{
public:
  uint8_t b, g, r, pad;
};

class CBGR48
{
public:
  uint16_t b, g, r, pad;
} gr_palette_std[COLORS_NUM] = {
    {0x0000, 0x0000, 0x0000, 0}, {0xAAAA, 0x0000, 0x0000, 0}, {0x0000, 0xAAAA, 0x0000, 0}, {0xAAAA, 0xAAAA, 0x0000, 0}, {0x0000, 0x0000, 0xAAAA, 0}, {0xAAAA, 0x0000, 0xAAAA, 0}, {0x0000, 0x5555, 0xAAAA, 0}, {0xAAAA, 0xAAAA, 0xAAAA, 0}, {0x5555, 0x5555, 0x5555, 0}, {0xFFFF, 0x5555, 0x5555, 0}, {0x5555, 0xFFFF, 0x5555, 0}, {0xFFFF, 0xFFFF, 0x5555, 0}, {0x5555, 0x5555, 0xFFFF, 0}, {0xFFFF, 0x5555, 0xFFFF, 0}, {0x5555, 0xFFFF, 0xFFFF, 0}, {0xFFFF, 0xFFFF, 0xFFFF, 0}};

#define DCF_COMPRESSED 1
#define DCF_PALETTE 2

class CDC
{
public:
  CDate cdt;
  int x0, y0,
      width, width_internal,
      height,
      flags;
  CBGR48 palette[COLORS_NUM];
  uint8_t *body;
};

CDC *DCNew(uint32_t width, uint32_t height)
{
  CDC *res = (CDC *)CAlloc(sizeof(CDC));
  res->width = width;
  res->width_internal = (width + 7) & ~7;
  res->height = height;
  res->body = (uint8_t *)CAlloc(res->width_internal * res->height);
  return res;
}

void DCDel(CDC *dc)
{ //Free dc, image body, rot mat and depth buf.
  if (!dc)
    return;
  Free(dc->body);
  Free(dc);
}

CDC *DCLoad(uint8_t *src, uint32_t *_size)
{ //Loads device context from mem.
  uint8_t *ptr = src;
  CArcCompress *arc;
  uint32_t body_size;
  CDC *res = (CDC *)CAlloc(sizeof(CDC));
  memcpy(res, ptr, sizeof(CDC) - sizeof(uint8_t *) - COLORS_NUM * sizeof(CBGR48));
  ptr += sizeof(CDC) - sizeof(uint8_t *) - COLORS_NUM * sizeof(CBGR48);
  if (res->flags & DCF_PALETTE)
  {
    memcpy(&res->palette, ptr, COLORS_NUM * sizeof(CBGR48));
    ptr += COLORS_NUM * sizeof(CBGR48);
  }
  else
    memcpy(&res->palette, gr_palette_std, sizeof(gr_palette_std));

  body_size = res->width_internal * res->height;
  if (res->flags & DCF_COMPRESSED)
  {
    res->flags &= ~DCF_COMPRESSED;
    arc = (CArcCompress *)ptr;
    res->body = arc->ExpandBuf();
    ptr += arc->compressed_size;
  }
  else
  {
    res->body = (uint8_t *)malloc(body_size);
    memcpy(res->body, ptr, body_size);
    ptr += body_size;
  }
  *_size = ptr - src;
  return res;
}

bool GrBlot(CDC *dc, int x, int y, CDC *img)
{
  int j, k, k1, w1, h1, w2, h2;
  if (x < 0)
    w1 = -x;
  else
    w1 = 0;
  if (y < 0)
    h1 = -y;
  else
    h1 = 0;
  w2 = img->width;
  h2 = img->height;
  if (x + w2 > dc->width)
    w2 = dc->width - x;
  if (y + h2 > dc->height)
    h2 = dc->height - y;
  if (w1 < w2 && w2 <= img->width && h1 < h2 && h2 <= img->height)
  {
    k = h1 * img->width_internal + w1;
    k1 = (h1 + y) * dc->width_internal + x + w1;
    for (j = h1; j < h2; j++)
    {
      memcpy(dc->body + k1, img->body + k, w2 - w1);
      k += img->width_internal;
      k1 += dc->width_internal;
    }
    return true;
  }
  else
    return false;
}

#define BMP_COLORS_NUM 16

class CFileBMP
{
public:
  uint16_t type;
  uint32_t file_size;
  uint32_t reserved;
  uint32_t data_offset;

  uint32_t header_size;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t bit_cnt;
  uint32_t compression;
  uint32_t image_size;
  uint32_t x_pixs_per_meter;
  uint32_t y_pixs_per_meter;
  uint32_t colors_used;
  uint32_t important_colors;

  CBGR24 palette[BMP_COLORS_NUM];
};

CFileBMP *BMPRLE4To(CDC *dc)
{ //To Windows RLE4 bit BMP.
  uint8_t *src, *ptr, *start;
  int i, x, y, w = dc->width, cnt, pattern;
  CBGR48 palette[COLORS_NUM];
  CFileBMP *bmp = (CFileBMP *)CAlloc(sizeof(CFileBMP) +
                                     2 * (dc->width + 1) * dc->height);
  bmp->type = 'B' + ('M' << 8);
  bmp->planes = 1;
  bmp->data_offset = sizeof(CFileBMP);
  bmp->header_size = sizeof(CFileBMP) - BMP_COLORS_NUM * sizeof(CBGR24) - 14;
  bmp->width = dc->width;
  bmp->height = dc->height;
  bmp->bit_cnt = 4;
  bmp->compression = 2; //RLE4
  memcpy(&palette, &gr_palette_std, sizeof(gr_palette_std));
  for (i = 0; i < BMP_COLORS_NUM; i++)
  {
    bmp->palette[i].b = palette[i].b >> 8;
    bmp->palette[i].g = palette[i].g >> 8;
    bmp->palette[i].r = palette[i].r >> 8;
    bmp->palette[i].pad = 0;
  }
  start = ptr = (uint8_t *)bmp + bmp->data_offset;
  for (y = dc->height - 1; y >= 0; y--)
  {
    src = y * dc->width_internal + dc->body;
    x = 0;
    while (x < w)
    {
      pattern = ((src[0] & 15) << 4) + (src[1] & 15);
      if ((x + 1 < w) && ((src[0] & 15) == (src[1] & 15)))
      {
        src += 2;
        cnt = 2;
        x += 2;
        while ((x < w) && (cnt < UINT8_MAX))
        {
          if ((*src & 15) == (pattern & 15))
          {
            src++;
            cnt++;
            x++;
          }
          else
            break;
        }
      }
      else
      {
        src += 2;
        if (x + 1 < w)
          cnt = 2;
        else
          cnt = 1;
        x += 2;
      }
      *ptr++ = cnt;
      *ptr++ = pattern;
    }
    *ptr++ = 0;
    *ptr++ = 0;
  }
  bmp->image_size = ptr - start;
  bmp->file_size = sizeof(CFileBMP) + bmp->image_size;
  return bmp;
}

void BMPWrite(char *filename, CDC *dc)
{ //Window's BMP Files.
  CFileBMP *bmp = BMPRLE4To(dc);
  FILE *out_file;
  if (out_file = fopen(filename, "wb"))
  {
    fwrite(bmp, 1, bmp->file_size, out_file);
    fclose(out_file);
  }
  Free(bmp);
}

int MV2BMPLst(char *mv_print_fmt, char *dir_print_fmt, char *bmp_print_fmt,
              char *mp4_print_fmt, char *out_name, double fps = 30000.0 / 1001)
{ //Cvt GR list MV file to BMP lst
  uint32_t bmp_num, size;
  bool cont = true;
  int num = 0, remaining, mv_percent_cnt, bmp_percent_cnt, dir_percent_cnt,
      mp4_percent_cnt;
  CDC *dc, *base;
  uint8_t *src, *src_base;
  char st[512], st_dir[512], st_bmp[512], st_mp4[512];
  CDate cdt;
  FILE *mv_file, *lst_file = fopen("TOSZTEMP_VID_LIST.TXT", "wt");
  cdt.time = 0;
  cdt.date = INT32_MIN;

  if (mv_print_fmt)
    mv_percent_cnt = StrOcc(mv_print_fmt, '%');
  else
    mv_percent_cnt = 0;
  if (bmp_print_fmt)
    bmp_percent_cnt = StrOcc(bmp_print_fmt, '%');
  else
    bmp_percent_cnt = 0;
  if (dir_print_fmt)
    dir_percent_cnt = StrOcc(dir_print_fmt, '%');
  else
    dir_percent_cnt = 0;
  if (mp4_print_fmt)
    mp4_percent_cnt = StrOcc(mp4_print_fmt, '%');
  else
    mp4_percent_cnt = 0;
  do
  {
    if (mv_percent_cnt)
      sprintf(st, mv_print_fmt, num);
    else
      strcpy(st, mv_print_fmt);
    if (mv_file = fopen(st, "rb"))
    {
      if (dir_percent_cnt)
        sprintf(st_dir, dir_print_fmt, num);
      else
        strcpy(st_dir, dir_print_fmt);
      sprintf(st, "mkdir %s", st_dir);
      system(st);
      bmp_num = 0;

      remaining = FSize(mv_file);
      src = src_base = (uint8_t *)malloc(remaining + 1);
      fread(src_base, 1, remaining, mv_file);
      src_base[remaining] = 0;
      base = DCNew(GR_WIDTH, GR_HEIGHT);
      while (cont && remaining > 0)
      {
        dc = DCLoad(src, &size);
        if (!size)
        {
          cont = false;
          break;
        }
        GrBlot(base, dc->x0, dc->y0, dc);
        if (cdt.time == 0 && cdt.date == INT32_MIN)
        {
          cdt.time = dc->cdt.time;
          cdt.date = dc->cdt.date;
        }
        while (cdt.date < dc->cdt.date ||
               ((cdt.date == dc->cdt.date) && (cdt.time <= dc->cdt.time)))
        {
          if (bmp_percent_cnt)
            sprintf(st_bmp, bmp_print_fmt, bmp_num);
          else
            strcpy(st_bmp, bmp_print_fmt);
          sprintf(st, "%s/%s", st_dir, st_bmp);
          BMPWrite(st, base);
          if (cdt.time >= UINT32_MAX - (uint32_t)(CDATE_FREQ / fps) + 1)
            cdt.date++;
          cdt.time += (uint32_t)(CDATE_FREQ / fps);
          if (!bmp_percent_cnt)
          {
            cont = false;
            break;
          }
          bmp_num++;
        }
        DCDel(dc);
        src += size;
        remaining -= size;
      }
      DCDel(base);
      Free(src_base);
      fclose(mv_file);
    }
    else
      break;
    if (mp4_print_fmt)
    {
      if (mp4_percent_cnt)
        sprintf(st_mp4, mp4_print_fmt, num);
      else
        strcpy(st_mp4, mp4_print_fmt);
      sprintf(st, "ffmpeg -framerate %0.16f -i %s/%s "
                  "-c:v libx264 -vf \"scale=2*trunc(iw/2):-2,setsar=1\" "
                  "-profile:v main -pix_fmt yuv420p %s",
              fps, st_dir, bmp_print_fmt, st_mp4);
      system(st);
      fprintf(lst_file, "file '%s'\r\n", st_mp4);
    }
    num++;
    if (!mv_percent_cnt)
      break;
  } while (cont);

  if (lst_file)
  {
    fclose(lst_file);
    if (num && out_name)
    {
      sprintf(st, "ffmpeg -f concat -i TOSZTEMP_VID_LIST.TXT -c copy %s",
              out_name);
      system(st);
    }
  }
  system("rm -R TOSZTEMP*");
  return num;
}

int SND2MP4Lst(char *snd_print_fmt, char *mp4_print_fmt, char *out_name,
               int rate = 24000)
{ //Cvt SND lst to BMP lst
  int num = 0, snd_percent_cnt, mp4_percent_cnt, mkdir_percent_cnt;
  char st[512], st_snd[512], st_mp4[512];
  FILE *snd_file, *lst_file = fopen("TOSZTEMP_AUD_LIST.TXT", "wt");

  if (snd_print_fmt)
    snd_percent_cnt = StrOcc(snd_print_fmt, '%');
  else
    snd_percent_cnt = 0;
  if (mp4_print_fmt)
    mp4_percent_cnt = StrOcc(mp4_print_fmt, '%');
  else
    mp4_percent_cnt = 0;

  while (true)
  {
    if (snd_percent_cnt)
      sprintf(st_snd, snd_print_fmt, num);
    else
      strcpy(st_snd, snd_print_fmt);
    if (snd_file = fopen(st_snd, "rb"))
    {
      fclose(snd_file);
      if (mp4_percent_cnt)
        sprintf(st_mp4, mp4_print_fmt, num);
      else
        strcpy(st_mp4, mp4_print_fmt);
      sprintf(st, "ffmpeg -i %s -ar %d %s", st_snd, rate, st_mp4);
      system(st);
      fprintf(lst_file, "file '%s'\r\n", st_mp4);
    }
    else
      break;
    num++;
    if (!snd_percent_cnt)
      break;
  }

  if (lst_file)
  {
    fclose(lst_file);
    if (num && out_name)
    {
      sprintf(st, "ffmpeg -f concat -i TOSZTEMP_AUD_LIST.TXT -c copy %s",
              out_name);
      system(st);
    }
  }
  system("rm -R TOSZTEMP*");
  return num;
}

int main(int argc, char *argv[])
{
  char *in_name, *out_name, st[256],
      *mv_print_fmt, *snd_print_fmt;
  bool cvt_ascii, del_in = false;
  int i = 1, l;
  if (argc > i && !strcmp(argv[i], "-bmp"))
  {
    i++;
    if (argc > i)
      mv_print_fmt = argv[i++];
    else
      mv_print_fmt = (char *)"VID%03d.MV";
    if (argc > i)
      out_name = argv[i++];
    else
      out_name = (char *)"VIDEO.MP4";
    MV2BMPLst(mv_print_fmt, (char *)"TOSZTEMP%03d", (char *)"TOSZTEMP%06d.BMP",
              (char *)"TOSZTEMPVID%03d.MP4", out_name);
  }
  else if (argc > i && !strcmp(argv[i], "-snd"))
  {
    i++;
    if (argc > i)
      snd_print_fmt = argv[i++];
    else
      snd_print_fmt = (char *)"AUD%03d.SND";
    if (argc > i)
      out_name = argv[i++];
    else
      out_name = (char *)"AUDIO.MP4";
    SND2MP4Lst(snd_print_fmt, (char *)"TOSZTEMPAUD%03d.MP4", out_name);
  }
  else if (argc > i && !strcmp(argv[i], "-mp4"))
  {
    i++;
    if (argc > i)
      mv_print_fmt = argv[i++];
    else
      mv_print_fmt = (char *)"VID%03d.MV";
    if (argc > i)
      snd_print_fmt = argv[i++];
    else
      snd_print_fmt = (char *)"AUD%03d.SND";
    MV2BMPLst(mv_print_fmt, (char *)"TOSZTEMP%03d",
              (char *)"TOSZTEMP%06d.BMP", (char *)"TOSZTEMPVID%03d.MP4",
              (char *)"TOSZVIDEO.MP4");
    SND2MP4Lst(snd_print_fmt, (char *)"TOSZTEMPAUD%03d.MP4",
               (char *)"TOSZAUDIO.MP4");
    if (argc > i)
      out_name = argv[i++];
    else
      out_name = (char *)"MOVIE.MP4";
    sprintf(st, "ffmpeg -i TOSZVIDEO.MP4 -i TOSZAUDIO.MP4 %s", out_name);
    system(st);
    system("rm TOSZVIDEO.MP4");
    system("rm TOSZAUDIO.MP4");
  }
  else
  {
    if (argc > i && !strcmp(argv[i], "-ascii"))
    {
      cvt_ascii = true;
      i++;
    }
    else
      cvt_ascii = false;
    if (argc > i)
    {
      in_name = argv[i++];
      if (argc > i)
        out_name = argv[i++];
      else
      {
        strcpy(st, in_name);
        l = strlen(st);
        if (l > 2 && st[l - 1] == 'Z' && st[l - 2] == '.')
        {
          st[l - 2] = 0;
          del_in = true;
        }
        out_name = st;
      }
      if (argc > i)
        out_name = argv[i++];
      else
      {
        strcpy(st, in_name);
        l = strlen(st);
        if (l > 2 && st[l - 1] == 'Z' && st[l - 2] == '.')
        {
          st[l - 2] = 0;
          del_in = true;
        }
        out_name = st;
      }
      if (Cvt(in_name, out_name, cvt_ascii))
      {
        if (del_in)
        {
          sprintf(st, "rm %s", in_name);
          system(st);
        }
      }
      else
        printf("Fail: %s %s\r\n", in_name, out_name);
    }
    else
    {
      puts("TOSZ [-ascii] in_name [out_name]\r\n\r\n"
           "TOSZ expands a single TempleOS file.  "
           "The -ascii flag will convert "
           "nonstandard TempleOS ASCII "
           "characters to regular ASCII.\r\n\r\n");
      puts("TOSZ -bmp VIDEO.MV VIDEO.MP4\r\n"
           "TOSZ will expand MV files to a sequence of BMP files and, "
           "will call ffmpeg to convert BMP files to an MP4 file.\r\n\r\n");
      puts("TOSZ -snd AUDIO.SND AUDIO.MP4\r\n\r\n"
           "TOSZ will call ffmpeg to convert SND files "
           "to an MP4 file.\r\n");
      puts("TOSZ -mp4 VIDEO.MV AUDIO.SND MOVIE.MP4\r\n\r\n"
           "TOSZ will call ffmpeg to convert MV files and SND files "
           "to a combined MP4 file.\r\n");
    }
  }
  return EXIT_SUCCESS;
}
