#!/usr/bin/perl

$prog="~/Projects/unlzss/bin/Release/unlzss";


$#a=-1;

opendir ( DIR, "./SCRIPTS" );
while( ($filename = readdir(DIR))){
     if (!($filename eq "..") and !($filename eq "."))
	{
	$a[$#a+1]=$filename;
	}
}
closedir(DIR);

$#b=-1;
for ($i=0; $i<=$#a; $i++)
{
	@z=`grep pana ./SCRIPTS/$a[$i]`;
	if ($#z>-1)
	{
	$b[$i]=1;
	}
	else
	{
	$b[$i]=0;
	}
}

$#d=-1;
@c=`find ./ | grep -i tga`;
for ($i=0; $i<=$#c; $i++)
{
chomp($c[$i]);
$ind = rindex ($c[$i],"/");
$d[$i]= uc(substr ($c[$i],$ind+1));
}



for ($i=0; $i<=$#a; $i++)
{
@z = `grep -i \.tga ./SCRIPTS/$a[$i]`;
if ($#z>-1)
{
for ($j=0; $j<=$#z; $j++)
{
	chomp($z[$j]);
	$z[$j]=uc ($z[$j]);
	$fl=substr ( $z[$j], index($z[$j],".TGA")-8,12);
	for ($q=0; $q<=$#d; $q++)
	{
		if ($d[$q] eq $fl)
		{
			print "$prog \"$c[$q]\" \n";
			system("$prog \"$c[$q]\" ");
			
			$ind=rindex($c[$q],".");
			$fname=substr($c[$q],0,$ind);

			if ($b[$i] == 0)
			{
				system("convert \"$c[$q]\" \"$fname.png\"");
			}
			else
			{
				system("convert -transpose \"$c[$q]\" \"$fname.png\"");
			}
			system("rm \"$c[$q]\" ");
		
		last;
		}
	}
}
}
}



