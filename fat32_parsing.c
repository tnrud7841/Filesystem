#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int select_hextodec(unsigned char arr[], int start, int len) {
	unsigned char* temp_arr = (unsigned char*)malloc(len*2);
	int idx = 0,dec = 0;

	for (int i = start; i < start+len; i++, idx+=2) {
		temp_arr[idx] = (arr[i]&0xff) % 16;
		temp_arr[idx+1] = (arr[i] & 0xff) /16;
	}
	for (int i = 0; i < idx; i++) {
		dec += temp_arr[i] * (int)pow(16, i);
	}
	return dec;
}

void find_data_area(FILE *fp, unsigned char arr[], int offset, int byte_per_sector, int sp, int rs, int bool) {
	char ex[100] = "", name[100] = "";
	if (bool) printf("파일 이름 : ");
	else printf("폴더 이름: ");
	for (int i = 0; i<8;i++) {
		if (arr[i] == 32) break;
		name[i] = arr[i];
	}
	ex[0] = 46;
	for (int j = 8;j < 12;j++) {
		ex[j-7] = arr[j];
	}
	strcat(name, ex);
	printf("%s\n", name);
	int size = select_hextodec(arr, 28, 4);
	int sector = select_hextodec(arr, 26, 2) + select_hextodec(arr, 20, 2);
	int start = offset + (sector-rs)*sp * byte_per_sector;
	int idx = 0,num;

	FILE* out_file = fopen(name, "w");
	fp = fopen("fat32_image", "rb");
	while ((num = fgetc(fp)) != EOF) {
		if (idx >= start && idx < start + size) {
			fputc(num, out_file);
			//printf("%02x ", num);
		}
		else if (idx == start + size) {break;}
		idx++;
	}printf("\n");
}


void file_r_d(FILE* fp,int offset, int start, int size, int byte_per_sector, int sp, int root_directory_cluster) {
	int num, idx2 = 0;
	unsigned char* root_directory = (unsigned char*)malloc(size);
	fp = fopen("fat32_image", "rb");
	while ((num = fgetc(fp)) != EOF) {
		if (idx2 >= start && idx2 < start + size) {
			root_directory[idx2 - start] = (num & 0xff);
		}
		else if (idx2 == start + size) break;
		idx2++;
	}fclose(fp);
	int count = 11, file_or_dir;
	while (count < size) {
		count += 32;
		if (root_directory[count] == 32) {
			file_or_dir = 1;
			unsigned char arr[32];
			for (int i = count - 11, a = 0;i <= count + 20;i++, a++) {
				arr[a] = (root_directory[i] & 0xff);
			}
			find_data_area(fp, arr, offset, byte_per_sector, sp, root_directory_cluster, file_or_dir);
		}
		if (root_directory[count] == 16) {
			unsigned char arr[32];
			file_or_dir = 0;
			for (int i = count - 11, a = 0;i <= count + 20;i++, a++) {
				arr[a] = (root_directory[i] & 0xff);
			}
			int folder_sector = select_hextodec(arr, 26, 2) - select_hextodec(arr, 20, 2);
			int folder_area_start = offset + (folder_sector - root_directory_cluster) * sp * byte_per_sector;
			int folder_directory_size = 0, check = 0, idx = 0, temp;
			fp = fopen("fat32_image", "rb");
			int finished = 0;
			while ((temp = fgetc(fp)) != EOF) {
				if (idx >= folder_area_start) {
					folder_directory_size++;
					if (temp == finished) {
						check += 1;
						if (check > 10) break;
					}
					else check = 0;
				}
				idx++;
			}fclose(fp);
			find_data_area(fp, arr,offset, byte_per_sector, sp, root_directory_cluster, file_or_dir);
			file_r_d(fp, offset, folder_area_start, folder_directory_size, byte_per_sector, sp, root_directory_cluster);
		}
	}
	fclose(fp); 
}

int main() {
	FILE* fp;
	int num, idx = 0;
	if ((fp = fopen("fat32_image", "rb")) == NULL) {
		fputs("파일 열기 에러", stderr);
		exit(1);
	}
	char vbr_arr[90];
	int finished = 00;
	int check = 0, root_director_size = 0;
	int byte_per_sector, sector_per_cluster, reserved_sector, fat_size, fat_area_size, root_directory_cluster, root_directory_offset;
	while ((num = fgetc(fp)) != EOF) {
		if (idx < 90) {
			vbr_arr[idx] = num;
		}
		else if(idx == 90) {
			printf("[Boot Sector 영역]\n");
			for (int i = 0; i < 90;i++) {
				if (i % 16 == 0) printf("\n");
				printf("%02x ", vbr_arr[i] & 0xff);
			}printf("\n");
			byte_per_sector = select_hextodec(vbr_arr, 11, 2);
			sector_per_cluster = select_hextodec(vbr_arr, 13, 1);
			reserved_sector = select_hextodec(vbr_arr, 14, 2);
			fat_size = select_hextodec(vbr_arr, 16, 1);
			fat_area_size = select_hextodec(vbr_arr, 36, 4);
			root_directory_cluster = select_hextodec(vbr_arr, 44, 4);
			root_directory_offset = ((reserved_sector + fat_size * fat_area_size) * byte_per_sector);
			printf("byte_per_sector : %d\n", byte_per_sector);
			printf("sector_per_cluster : %d\n", sector_per_cluster);
			printf("reserved_sector : %d\n", reserved_sector);
			printf("fat_size : %d\n", fat_size);
			printf("fat_area_size : %d\n", fat_area_size);
			printf("root_directory_cluster : %d\n", root_directory_cluster);
			printf("root_directory_offset : %d\n", root_directory_offset);
			printf("\n[root directory 영역]\n");
		}
		else if (idx >= root_directory_offset) {
			if (idx % 16 == 0) printf("\n");
			printf("%02x ", num);
			root_director_size++;
			if (num == finished) { 
				check += 1; 
				if (check > 10) { printf("\n"); break; }
			}
			else check = 0;
		}
		idx++;
	}	fclose(fp); printf("\n");
	fp = fopen("fat32_image", "rb");
	printf("\[data area]\n");
	file_r_d(fp, root_directory_offset, root_directory_offset,root_director_size, byte_per_sector, sector_per_cluster, root_directory_cluster);
	fclose(fp);

	return 0;
}