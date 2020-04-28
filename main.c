#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <json-c/json.h>

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s)
{
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writeFunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

int main(int arg, char **argv)
{
	char api[] = "https://airapi.airly.eu/v2/measurements/nearest?indexType=AIRLY_CAQI&lat=50.288840&lng=18.677970&maxDistanceKM=3";
	char apiKey[] = "apikey: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	
	int pm25;
	int pm10;
	
	while (1)
	{
		CURL *curl;
		curl = curl_easy_init();
		
		if (curl)
		{
			struct string rawData;
		  init_string(&rawData);
			
			struct curl_slist *headerFields = NULL;
			headerFields = curl_slist_append(headerFields, apiKey);
			
			curl_easy_setopt(curl, CURLOPT_URL, api);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerFields);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawData);
			
			int resultCode = curl_easy_perform(curl);
			
			if (resultCode == CURLE_OK)
			{	
				struct json_object *parsedData;
				struct json_object *current;
				struct json_object *standards;
				
				parsedData = json_tokener_parse(rawData.ptr);
				json_object_object_get_ex(parsedData, "current", &current);
				json_object_object_get_ex(current, "standards", &standards);
				
				int standardsLen = json_object_array_length(standards);
				
				for (int i=0; i<standardsLen; i++)
				{
					struct json_object *standard = json_object_array_get_idx(standards, i);
					struct json_object *pollutant;
					struct json_object *percent;
					
					json_object_object_get_ex(standard, "pollutant", &pollutant);
					json_object_object_get_ex(standard, "percent", &percent);
					
					char *pollutantStr = json_object_get_string(pollutant);
					
					if (strcmp(pollutantStr, "PM25") == 0){
						pm25 = json_object_get_int(percent);
					}
					else if (strcmp(pollutantStr, "PM10") == 0){
						pm10 = json_object_get_int(percent);
					}
				}
				printf("pm2.5: %d%% pm10: %d%%\n", pm25, pm10);
			}
			else {
				printf("ERROR: %s\n", curl_easy_strerror(resultCode));
			}
			
			free(rawData.ptr);
		}
		curl_easy_cleanup(curl);
		
		sleep(1800);
	}	
		
	getchar();
	return 0;
}
