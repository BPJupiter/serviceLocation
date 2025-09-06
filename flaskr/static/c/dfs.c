// Compile with -lcjson

#include <stdio.h>
#include <cjson/cJSON.h>

int main(int argc, char* argv[]) {
  FILE* fp = fopen("../json/cities-GCS.json", "r");
  if (fp == NULL) {
    printf("Error: unable to open file.");
    return 1;
  }

  char buffer [1 << 18];
  int len = fread(buffer, 1, sizeof(buffer), fp);
  fclose(fp);

  cJSON *json = cJSON_Parse(buffer);
  if (json == NULL) {
    return 1;
  }

  cJSON *type = cJSON_GetObjectItemCaseSensitive(json, "type");
  printf("%s", type->valuestring);
  cJSON_Delete(json);
  return 0;
}
