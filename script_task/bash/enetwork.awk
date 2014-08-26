BEGIN {
	printf("rxerr/s\ttxerr/s\trxdrop/s\ttxdrop/s\n");
}{
	if($0 ~ /Average/) {
		printf("%s\t%s\t%s\t\t%s\n", $3, $4, $6, $7);
	}
} END {
}
