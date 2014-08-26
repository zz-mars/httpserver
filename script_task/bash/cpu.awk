BEGIN{
	b_single_cpu = 0;
	printf("---------------------- cpu usage ----------------------\n");
}{
	if($0 ~ /all/) {
		printf("cpu #all : %2.2f%% ", 100.0 - $12);
		for(i=0;i<$12;i+=2) {
			printf("|");
		}
		printf("\n");
		b_single_cpu = 1;
	} else if(b_single_cpu == 1) {
		printf("cpu #%3d : %2.2f%% ", $3, 100.0 - $12);
		for(i=0;i<$12;i+=2) {
			printf("|");
		}
		printf("\n");
	}
} END{
}
