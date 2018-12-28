// 2013013017 윤신웅
// cachesim.c
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MAX 100
#define SIZE 67108864 //64MB

// memory, cache
int* memory;
int** cache;
int L1_size, L1_line_size, set; // cache setting value
double clock_rate, cycle_sec; // clock setting value

// functions
void memory_read(int, int, int, int);
void cache_to_mem(int, int, int, int);

// set-associative functions
int set_cache_index(int * target_cache_index,int set, int byte_address, int cache_index, int cache_tag);

// main function
int main(int ac, char * av[]) {
	// local variable
	time_t starttime = 0, finishtime = 0;
	FILE* fp1;
	FILE* fp2;
	int mem_access_latency; 
	struct timeval bgn,end;		
	double time1;
	char msgbuf[MAX]; // string buf
	int memory_access_count=0, cache_access_count=0, miss_count = 0; // for count 
	int cache_index_size, cache_index, cache_tag, cache_byte_offset; // cache field setting value
	int byte_address, data; // process value

	// 시간 처리
	starttime = clock();

	// argument error 처리
	if(ac !=3){
		fprintf(stderr,"You must use 2 Argument!!! \nex) ./cachesim cachesim.conf mem_access.txt\n");
		exit(1);
	}
	// 파일 열기
	if((fp1 = fopen(av[1], "r"))== NULL){
		fprintf(stderr, "FILE OPEN ERROR!\n");
		exit(1);
	}
	if((fp2 = fopen(av[2], "r"))== NULL){
		fprintf(stderr, "FILE OPEN ERROR!\n");
		exit(1);
	}
	// ******************************** 1. 파일 열어서 차례대로 설정값 읽어오기
	// clock rate
	fscanf(fp1, "%s", msgbuf);
	fscanf(fp1, "%lf", &clock_rate);
	printf("%s %.0lf\n",msgbuf, clock_rate);

	// mem_access_latency
	fscanf(fp1, "%s", msgbuf);
	fscanf(fp1, "%d", &mem_access_latency);
	printf("%s %d\n", msgbuf, mem_access_latency);

	// L1_size
	fscanf(fp1, "%s", msgbuf);
	fscanf(fp1, "%d", &L1_size);
	printf("%s %d\n",msgbuf, L1_size);

	// L1_line_size
	fscanf(fp1, "%s", msgbuf);
	fscanf(fp1, "%d", &L1_line_size);
	printf("%s %d\n", msgbuf, L1_line_size);

	// set_associativity
	fscanf(fp1, "%s", msgbuf);
	fscanf(fp1, "%d", &set);
	printf("%s %d\n\n", msgbuf, set);
		
	// 단위 설정
	cache_index_size = L1_size / (L1_line_size); // cache 접근 시 활용할 size는 전체 크기 / line size
	cycle_sec = 1.0 / (clock_rate); // clock_cycle_time = 1 / clock_rate
	

	// 메모리 메모리 할당(64MB)
	memory = (int*)calloc(SIZE, sizeof(int));
	
	// ******************************** 2. 메모리 읽고 쓰기 
	gettimeofday(&bgn, NULL); // 시간 측정 시작
	while(!feof(fp2)){
		// 필드 1 모드 읽기
		char ch;
		fscanf(fp2,"%c ", &ch);
		// lw
		if(ch == 'R'){
			fscanf(fp2,"%d ", &byte_address); // 필드 2 주소 읽기
			//printf("%d\n",memory[byte_address]); // read
		}
		// sw
		if(ch == 'W'){
			fscanf(fp2,"%d ", &byte_address); // 필드 2 주소 읽기
			fscanf(fp2,"%d " , &data); // 필드 3 범위 읽기
			memory[byte_address] = data; // write
		}
		memory_access_count++;
	}
	gettimeofday(&end, NULL); // 시작 측정 끝
	fclose(fp2); // 파일 닫기

	// 시간 계산
	time1 = end.tv_sec + end.tv_usec / 1000000.0 - bgn.tv_sec - bgn.tv_usec / 1000000.0;
	time1 = (time1 * 250 * 1000000000)/memory_access_count; // ns 단위
	//printf("%.1fns\n",time1);
	printf("Average memory access latency without cache: %.1fns\n\n",(cycle_sec*250)); // ns단위 걸린 시간 출력
	
	// 메모리 재할당(초기화)
	free(memory); // 메모리 비워주기
	memory = (int*)calloc(SIZE, sizeof(int)); // 메모리 메모리 할당(64MB)

	memory_access_count = 0;
	// 파일 다시 열기
	if((fp2 = fopen(av[2], "r"))== NULL){
		fprintf(stderr, "FILE OPEN ERROR!\n");
		exit(1);
	}
	// 캐쉬 메모리 할당(cache_index_size 크기) 2차원 배열
	cache = (int**)calloc(cache_index_size, sizeof(int*));
	for (int i = 0; i < cache_index_size; i++) {
		cache[i] = (int*)calloc(((L1_line_size/4) + 2), sizeof(int));
	}

	// ******************************** 3~5. 캐쉬 읽고 쓰기
	while (!feof(fp2)) {
		// 필드 1 모드 읽기
		char ch;
		fscanf(fp2,"%c ", &ch);
		fscanf(fp2, "%d ", &byte_address);// 필드 2 주소 읽기 (32bit - int)	

		// 메모리 주소 32 bit field 구분
		cache_byte_offset = byte_address % (L1_line_size); // byte-offset 은 int data 공간에 따라		
		cache_index = (byte_address / (L1_line_size)) % (cache_index_size/set); // byte-offset 다음 size
		cache_tag = (byte_address / (L1_line_size)) / (cache_index_size/set);   // tag 값은 나머지 값
		printf("%d ", cache_byte_offset);
	
		// Direct mapped
		if(set==1){
			// lw
			if (ch == 'R') {
				// valid == 0 (miss) 
				if (cache[cache_index][0] == 0) {
					memory_read(byte_address, cache_index, cache_tag, cache_byte_offset); // memory read
					cache_access_count++; // access_count 
					miss_count++; // miss
					memory_access_count++;
				}

				// valid == 1, tag not same (miss)
				else if(cache[cache_index][1] != cache_tag){
					cache_to_mem(byte_address, cache_index, cache_index_size, cache_byte_offset); // 기존 값 옮기기
					memory_read(byte_address, cache_index, cache_tag, cache_byte_offset); // memory read
					cache_access_count++; // access_count 
					miss_count++; // miss
					memory_access_count++;
					if(L1_line_size > 4)
						memory_access_count++;
				}

				// valid == 1, tag same (hit)
				else {
					cache_access_count++; // access_count 
				}
			}

			// sw
			else if(ch == 'W'){
				fscanf(fp2,"%d ", &data); // 필드 3 데이터 읽기

				// valid == 0 (miss)
				if (cache[cache_index][0] == 0) {
					memory_read(byte_address, cache_index, cache_tag, cache_byte_offset); // memory read
					cache[cache_index][2 + cache_byte_offset/4] = data; // access 1번처리
					cache_access_count++; // access_count
					miss_count++; // miss
					memory_access_count++;
				}
				
				// valid == 1, tag not same (miss)
				else if (cache[cache_index][1] != cache_tag) {
					cache_to_mem(byte_address, cache_index, cache_index_size, cache_byte_offset);
					memory_read(byte_address, cache_index, cache_tag, cache_byte_offset); // memory read
					cache[cache_index][2 + cache_byte_offset/4] = data;
					cache_access_count++; // access_count
					miss_count++; // miss
					if(L1_line_size > 4)
						memory_access_count++;
				}

				// valid == 1, tag same (hit)
				else {
					cache[cache_index][2 + cache_byte_offset/4] = data;
					cache_access_count++; // access_count 
				}
			}
			// mode error!
			else {
				fprintf(stderr, "MODE ERROR!!!it is not R or W!!!\n");
				exit(1);
			}
		} // direct mapped end

		// set-associative
		else if( set >1){
			int target_cache_index; // set 내 위치 찾기
			cache_index *=set;
			int start_cache_index = cache_index - (cache_index % set); // set 시작 위치
			int end_cache_index = start_cache_index + set -1; 
			int set_check = set_cache_index(&target_cache_index, set, byte_address, cache_index, cache_tag) ;
			//printf("%d~%d ", start_cache_index, end_cache_index);
			// lw
			if (ch == 'R') {
				// valid == 0 (miss) 
				if (set_check == -1) {
					memory_read(byte_address, target_cache_index, cache_tag, cache_byte_offset); // memory read
					cache_access_count++; // access_count 
					miss_count++; // miss
					memory_access_count++;
				}

				// valid == 1, tag not same (miss)
				else if(set_check == -2){
					cache_to_mem(byte_address, end_cache_index, cache_index_size, cache_byte_offset); // set 내에 맨 끝 index 기존 값 옮기기

					// set 내 값 한칸씩 이동
					for(int i=end_cache_index; i!= start_cache_index;i--){
						for(int j=0;j<(L1_line_size/4) + 2; j++)
							cache[i][j] = cache[i-1][j];
					}

					memory_read(byte_address, start_cache_index, cache_tag, cache_byte_offset); // 첫 값에다가 memory read
					cache_access_count++; // access_count 
					miss_count++; // miss
					memory_access_count++;
					if(L1_line_size > 4)
						memory_access_count++;
				}

				// valid == 1, tag same (hit)
				else {
					cache_access_count++; // access_count 
				}
			}

			// sw
			else if(ch == 'W'){
				fscanf(fp2,"%d ", &data); // 필드 3 데이터 읽기

				// valid == 0
				if (set_check == -1) {
					memory_read(byte_address, target_cache_index, cache_tag, cache_byte_offset); // memory read
					cache[target_cache_index][2 + cache_byte_offset/4] = data; // access 1번처리
					cache_access_count++; // access_count
					miss_count++; // miss
					memory_access_count++;
				}
				
				// valid == 1, tag not same
				else if (set_check == -2) {
					cache_to_mem(byte_address, end_cache_index, cache_index_size, cache_byte_offset); // set 내에 맨 끝 index 기존 값 옮기기

					// set 내 값 한칸씩 이동
					for(int i=end_cache_index ; i!= start_cache_index;i--){
						for(int j=0;j<(L1_line_size/4) + 2; j++)
							cache[i][j] = cache[i-1][j];
					}

					memory_read(byte_address, start_cache_index, cache_tag, cache_byte_offset); // memory read
					cache[start_cache_index][2 + cache_byte_offset/4] = data;
					cache_access_count++;  // access_count
					miss_count++; // miss
					memory_access_count++;
					if(L1_line_size > 4)
						memory_access_count++;
				}

				// valid == 1, tag same
				else {
					cache[target_cache_index][2 + cache_byte_offset/4] = data;
					cache_access_count++; // access_count 
				}
			}
			// mode error!
			else {
				fprintf(stderr, "MODE ERROR!!!\n");
				exit(1);
			}
		} // set-associative end
	}


	// 출력 처리
	printf("*L1 Cache Contents\n");
	for (int i = 0; i < cache_index_size; i++) {
		if(i %set == 0){		
			printf("%d: ", i/set);
			// 1이 아닐때만 줄바꿈	
			if(set != 1)
				printf("\n");
		}

		// set 단위 출력
		// block 부터만 출력
		for (int j = 2; j < (L1_line_size/4) + 2; j++) {
			printf("%08x ", cache[i][j]);
		}
		printf("\n");
		
	}
	finishtime = clock(); // clcok
	printf("\nTotal program run time: %0.1f seconds \n",(float)(finishtime-starttime)/(CLOCKS_PER_SEC));
	printf("L1 total access: %d\n", cache_access_count);
	printf("L1 total miss count: %d\n", miss_count);
	printf("L1 miss rate: %.2lf%%\n", ((double)miss_count / cache_access_count) * 100);
	printf("Average memory access latency: %.1lfns\n",(((double)(memory_access_count * mem_access_latency) + (cache_access_count * 2* set))/(double)cache_access_count) * cycle_sec  );
	  
	// 메모리 할당 해제
	free(memory);
	for (int i = 0; i < cache_index_size; i++) {
		free(cache[i]);
	}
	free(cache);

	// 파일 닫기
	fclose(fp1);
	fclose(fp2);

	return 0;
}

// direct mapped function
// memory로 부터 cache로 읽어오기
void memory_read(int byte_address, int cache_index, int cache_tag, int cache_byte_offset) {

	cache[cache_index][0] = 1; //valid를 1로 설정한다. 
	cache[cache_index][1] = cache_tag;// tag를 설정한다. 
 
	byte_address -=cache_byte_offset;

	for (int i = 2; i < 2 + (L1_line_size / 4); i++) {
		cache[cache_index][i] = memory[byte_address + 4 * (i - 2)];
	}

}
// cache에 있는 값을 memory에 쓰기
void cache_to_mem(int byte_address, int cache_index, int cache_index_size, int cache_byte_offset) {

	byte_address -=cache_byte_offset;

	// 반복문으로 해당 메모리 블록에 값 할당
	for (int i = 2; i < 2 + L1_line_size / 4; i++) {
		memory[byte_address + 4 * (i - 2)] = cache[cache_index][i];

	}
}

// set-associative functions
// set 내에 해당 cache index 찾기, set 내에서 해당 값이 있다(hit) -> return index, 
// set 내에 empty 공간 있다(miss) -> reuturn -1 , set이 전부 다 찼다(miss) -> return -2 
int set_cache_index(int * target_cache_index, int set, int byte_address, int cache_index, int cache_tag){
	int start_cache_index = cache_index - (cache_index % set); // set 시작 위치
	int i;
	// set 동안 반복
	for(i = 0; i<set;i++){
		// valid == 1, tag same (hit)
		if(cache[start_cache_index+i][0] == 1 && cache[start_cache_index+i][1] == cache_tag ){
			*target_cache_index = (start_cache_index+i); 
			return 0;// hit return now_index
		}
		// valid == 0, set has empty space (miss)
		else if(cache[start_cache_index+i][0] == 0){
			*target_cache_index = (start_cache_index+i);
			return -1; // set empty miss return -1		
		}
		// valid == 1, tag not same, set is full (miss)		
		else if(cache[start_cache_index+i][0] == 1 && cache[start_cache_index+i][1] != cache_tag )
			; 
	} 
	
	*target_cache_index = (start_cache_index+i-1);
	return -2;// set full miss return -1 
}
