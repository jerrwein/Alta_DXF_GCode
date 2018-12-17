BEGIN {
	# Usage: $> gawk -f param_extract.awk ../../src/*.c  (or your path to *.c files containing params)
	# Note: Manually place the generated tool_param_table.c' in your destination folder. 
	FS = " "
	Output_Filename = "tool_param_table.c"
	Input_Filename = "unknown";
	input_file_idx = 0;
	LinessExtracted = 1;
	last_x_abs = 0.0000;
	last_y_abs = 0.0000;
}
{
	# printf ("Info1: %s: #1: %s #2: %s\n", $0, $1, $2);

	if (match($1, /N/) && match($2, /G0/) && !match($3, /Z/))
	{
		# Strip all leading X,Y,I & J characters.
		gsub(/[X]/, "", $3);
		gsub(/[Y]/, "", $4);
	#	printf ("InfoXY: %s\n", $0);
		
		# Calculate deltas
		this_x = $3;
		this_y = $4;
		delta_x = this_x - last_x_abs;
		delta_y = this_y - last_y_abs;

#		printf ("Info-%s: last: %s %s, this: %s %s, delta: %s %s\n", $1, last_x_abs, last_y_abs, this_x, this_y, delta_x, delta_y);
		print $0;
		last_x_abs = $3;
		last_y_abs = $4;
#		printf (" => %s, %s X:%s Y%s \n", $1, $2, delta_x, delta_y);

		if (match($2, "G00") || match($2, "G01"))
		{
#			print $0;
			printf (" => %s %s X%s Y%s \n", $1, $2, delta_x, delta_y);
			# Strip all leading X,Y,I & J characters.
#			gsub(/[X]/, "", $3);
#			gsub(/[Y]/, "", $4);
#			printf ("InfoXY: %s\n", $0);
#			this_x = $3;
#			this_y = $4;
		}
		else if (match($2, "G02") || match($2, "G03"))
		{
#			print $0;
			printf (" => %s %s X%s Y%s %s %s \n", $1, $2, delta_x, delta_y, $5, $6);
#			gsub(/[I]/, "", $5);
#			gsub(/[J]/, "", $6);
#			last_i = $5;
#			last_j = $6;
#			printf ("Info-IJ: %s %s\n", last_i, last_j);
		}
		LinessExtracted++;
	}
}

END {
	printf ("Lines converted: %d\n", LinessExtracted);
}
 