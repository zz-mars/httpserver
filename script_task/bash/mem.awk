BEGIN{
	printf("---------------------- mem usage ----------------------\n");
}{
	if ($0 ~ /Mem/) {
		total = $2;
		used = $3;
		free = $4;
		buf = $6;
		cache = $7;
		buf_cache = buf + cache;
		used_percentage = 100.0 * used / total;
		buf_cache_percentage = 100.0 * buf_cache / total;
		printf("total_mem : %dKB\n", total);
		printf("used      : %2.2f%% ", used_percentage);
		for(i=0;i<used_percentage;i+=2) {
			printf("|");
		}
		printf("\nbuf_cache : %2.2f%% ", buf_cache_percentage);
		for(i=0;i<buf_cache_percentage;i+=2) {
			printf("|");
		}
		printf("\n");
	} else if($0 ~ /Swap/) {
		swap_total = $2;
		swap_used = $3;
		swap_used_percentage = 100.0 * swap_used / swap_total;
		printf("swap      : %dKB\n", swap_total);
		printf("swap_used : %2.2f%% ", swap_used_percentage);
		for(i=0;i<swap_used_percentage;i+=2) {
			printf("|");
		}
		printf("\n");
	}
}END{}

