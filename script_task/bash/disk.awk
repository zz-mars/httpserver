BEGIN {
	b_direct_output = 0;
	printf("---------------------- disk usage ----------------------\n");
} {
	if($0 ~ /Device/) {
		printf("%s\n", $0);
		b_direct_output = 1;
	}else if(b_direct_output == 1) {
		if ($0 !~ /^$/) {
			printf("%s\n", $0);
		}
	}
} END {
}

