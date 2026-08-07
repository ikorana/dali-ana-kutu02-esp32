#include "storage_little.h"

const char *LITTLETAG = "STORAGE_LITTLE";

bool StorageLittle::format(void)
{
   esp_littlefs_format("storage");
   return true;
}

bool StorageLittle::init(void)
{   
    bool err = true;
    esp_vfs_littlefs_conf_t conf = {};
      conf.base_path = "/config";
      conf.partition_label = "storage";
      conf.format_if_mount_failed = true;
      conf.dont_mount = false;

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(LITTLETAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(LITTLETAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(LITTLETAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(LITTLETAG, "Failed to get LittleFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_littlefs_format(conf.partition_label);
    } else {
        ESP_LOGI(LITTLETAG, "Partition size: total: %d, used: %d", total, used);
    }

    return err;
}

bool StorageLittle::file_search(const char *name)
{
  struct stat st; 
  if (stat(name, &st) != 0) {
        ESP_LOGE(LITTLETAG, "%s not FOUND", name);
        return false;
  } else { 
    return true; 
  }   
}

bool StorageLittle::file_empty(const char *name)
{
  if (file_search(name)) 
  {
    FILE *fd = fopen(name, "w");
    if (fd == NULL) {
      ESP_LOGE(LITTLETAG, "%s not created", name);
      return false;
    }
    fclose(fd);
    return true;
  } 
  return false;
}

bool StorageLittle::file_create(const char *name, uint16_t size)
{
    FILE *fd = fopen(name, "w");
    if (fd == NULL) {
      ESP_LOGE(LITTLETAG, "%s not created", name);
      return false;
    }
    fseek(fd, (1*size), SEEK_SET);
    char *ff = (char *)calloc(1, size);
    fwrite(ff, 1, size, fd);
    free(ff);
    fclose(fd);
    return true;
}

bool StorageLittle::file_control(const char *name)
{
    if (!file_search(name)) {
        FILE *fd = fopen(name, "w");
        if (fd == NULL) { ESP_LOGE(LITTLETAG, "FC %s not created", name); return false; }
        fclose(fd);
        return false;
    }                      
  return true;
}

int StorageLittle::file_size(const char *name)
{
    FILE *fd = fopen(name, "r+");
    if (fd == NULL) { ESP_LOGE(LITTLETAG, "FZ %s not open", name); return 0; }
    fseek(fd, 0, SEEK_END);
    int file_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    fclose(fd);
    return file_size;
}

bool StorageLittle::write_file(const char *name, void *flg, uint16_t size, uint8_t obj_num)
{
     FILE *fd = fopen(name, "r+");
     if (fd == NULL) { ESP_LOGE(LITTLETAG, "WR %s not open", name); return false; }
     fseek(fd, (obj_num * size), SEEK_SET);
     fwrite(flg, 1, size, fd);
     fflush(fd);
     fsync(fileno(fd));
     fclose(fd);
     return true; 
}

bool StorageLittle::read_file(const char *name, void *flg, uint16_t size, uint8_t obj_num)
{
     FILE *fd = fopen(name, "rb");
     if (fd == NULL) { ESP_LOGE(LITTLETAG, "RD %s not open", name); return false; }
     fseek(fd, (obj_num * size), SEEK_SET); 
     size_t by = fread(flg, 1, size, fd);
     if (by != size) {
         ESP_LOGE(LITTLETAG, "RD %s failed to read complete object", name);
         fclose(fd);
         return false;
     }
     fclose(fd);
     return true;
}

#define EOS '\0'

const char *StorageLittle::rangematch(const char *pattern, char test, int flags)
{
  int negate, ok;
  char c, c2;

  if ( (negate = (*pattern == '!' || *pattern == '^')) ) ++pattern;
  if (flags & 0x10) test = tolower((unsigned char)test); // FNM_CASEFOLD manually as 0x10

  for (ok = 0; (c = *pattern++) != ']';) {
    if (c == '\\' && !(flags & 0x01)) c = *pattern++;
    if (c == EOS) return (NULL);
    if (flags & 0x10) c = tolower((unsigned char)c);

    if (*pattern == '-' && (c2 = *(pattern+1)) != EOS && c2 != ']') {
      pattern += 2;
      if (c2 == '\\' && !(flags & 0x01)) c2 = *pattern++;
      if (c2 == EOS) return (NULL);
      if (flags & 0x10) c2 = tolower((unsigned char)c2);
      if ((unsigned char)c <= (unsigned char)test && (unsigned char)test <= (unsigned char)c2) ok = 1;
    }
    else if (c == test) ok = 1;
  }
  return (ok == negate ? NULL : pattern);
}

int StorageLittle::fnmatch(const char *pattern, const char *string, int flags)
{
  const char *stringstart;
  char c, test;

  for (stringstart = string;;)
    switch (c = *pattern++) {
    case EOS:
      if ((flags & 0x08) && *string == '/') return (0);
      return (*string == EOS ? 0 : 1);
    case '?':
      if (*string == EOS) return (1);
      if (*string == '/' && (flags & 0x02)) return (1);
      if (*string == '.' && (flags & 0x04) && (string == stringstart || ((flags & 0x02) && *(string - 1) == '/'))) return (1);
      ++string;
      break;
    case '*':
      c = *pattern;
      while (c == '*') c = *++pattern;
      if (*string == '.' && (flags & 0x04) && (string == stringstart || ((flags & 0x02) && *(string - 1) == '/'))) return (1);
      if (c == EOS) return ((flags & 0x02) && strchr(string, '/') != NULL ? 1 : 0);
      while ((test = *string) != EOS) {
        if (!fnmatch(pattern, string, flags & ~0x04)) return (0);
        if ((test == '/') && (flags & 0x02)) break;
        ++string;
      }
      return (1);
    case '[':
      if (*string == EOS || ((*string == '/') && (flags & 0x02))) return (1);
      if ((pattern = rangematch(pattern, *string, flags)) == NULL) return (1);
      ++string;
      break;
    case '\\':
      if (!(flags & 0x01)) { if ((c = *pattern++) == EOS) { c = '\\'; --pattern; } }
      // FALLTHROUGH
    default:
      if (c != *string) {
          if (!((flags & 0x10) && (tolower((unsigned char)c) == tolower((unsigned char)*string)))) return (1);
      }
      string++;
      break;
    }
  return 0;
}

void StorageLittle::list(const char *path, const char *match) {
    DIR *dir = NULL;
    struct dirent *ent;
    char type;
    char size[12];
    char tpath[255];
    char tbuffer[80];
    struct stat sb;
    struct tm *tm_info;
    int statok;

    printf("\nLittleFS List of Directory [%s]\n", path);
    printf("-----------------------------------\n");
    dir = opendir(path);
    if (!dir) {
        printf("Error opening directory\n");
        return;
    }

    uint64_t total = 0;
    int nfiles = 0;
    printf("T  Size      Date/Time         Name\n");
    printf("-----------------------------------\n");
    while ((ent = readdir(dir)) != NULL) {
        sprintf(tpath, "%s", path);
        if (path[strlen(path)-1] != '/') strcat(tpath, "/");
        strcat(tpath, ent->d_name);
        tbuffer[0] = '\0';

        if ((match == NULL) || (fnmatch(match, tpath, 0x04) == 0)) {
            statok = stat(tpath, &sb);
            if (statok == 0) {
                tm_info = localtime(&sb.st_mtime);
                strftime(tbuffer, 80, "%d/%m/%Y %R", tm_info);
            }
            else sprintf(tbuffer, "                ");

            if (ent->d_type == DT_REG) {
                type = 'f';
                nfiles++;
                if (statok) strcpy(size, "       ?");
                else {
                    total += sb.st_size;
                    if (sb.st_size < (1024*1024)) sprintf(size, "%8d", (int)sb.st_size);
                    else if ((sb.st_size/1024) < (1024*1024)) sprintf(size, "%6dKB", (int)(sb.st_size / 1024));
                    else sprintf(size, "%6dMB", (int)(sb.st_size / (1024 * 1024)));
                }
            }
            else {
                type = 'd';
                strcpy(size, "       -");
            }

            printf("%c  %s  %s  %s\r\n",
                type,
                size,
                tbuffer,
                ent->d_name
            );
        }
    }
    if (total) {
        printf("-----------------------------------\n");
    	if (total < (1024*1024)) printf("   %8d", (int)total);
    	else if ((total/1024) < (1024*1024)) printf("   %6dKB", (int)(total / 1024));
    	else printf("   %6dMB", (int)(total / (1024 * 1024)));
    	printf(" in %d file(s)\n", nfiles);
    }
    printf("-----------------------------------\n");

    closedir(dir);
}
