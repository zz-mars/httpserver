BEGIN {
	printf("iface\trxpck/s\ttxpck/s\trxkB/s\ttxkB/s\n");
}{
	if($0 ~ /Average/) {
		printf("%s\t%s\t%s\t%s\t%s\n", $2, $3, $4, $5, $6);
	}
} END {
}
