# /= operator gives same result as / 
/a|b|c/	{
	print
	for (i = $1; (i /= 10)>= 1; )
		print " ", i
}


##Input
17379	mel
16693	bwk	me
16116	ken	him	someone else
15713	srb
11895	lem
10409	scj
10252	rhm
 9853	shen
 9748	a68
 9492	sif
 9190	pjw
 8912	nls
 8895	dmr
 8491	cda
 8372	bs
 8252	llc
 7450	mb
 7360	ava
 7273	jrv
 7080	bin
 7063	greg
 6567	dict
 6462	lck
 6291	rje
 6211	lwf
 5671	dave
 5373	jhc
 5220	agf
 5167	doug
 5007	valerie
 3963	jca
 3895	bbs
 3796	moh
 3481	xchar
 3200	tbl
 2845	s
 2774	tgs
 2641	met
 2566	jck
 2511	port
 2479	sue
 2127	root
 1989	bsb
 1989	jeg
 1933	eag
##Output
16693	bwk	me
  1669.3
  166.93
  16.693
  1.6693
15713	srb
  1571.3
  157.13
  15.713
  1.5713
10409	scj
  1040.9
  104.09
  10.409
  1.0409
 9748	a68
  974.8
  97.48
  9.748
 8491	cda
  849.1
  84.91
  8.491
 8372	bs
  837.2
  83.72
  8.372
 8252	llc
  825.2
  82.52
  8.252
 7450	mb
  745
  74.5
  7.45
 7360	ava
  736
  73.6
  7.36
 7080	bin
  708
  70.8
  7.08
 6567	dict
  656.7
  65.67
  6.567
 6462	lck
  646.2
  64.62
  6.462
 5671	dave
  567.1
  56.71
  5.671
 5373	jhc
  537.3
  53.73
  5.373
 5220	agf
  522
  52.2
  5.22
 5007	valerie
  500.7
  50.07
  5.007
 3963	jca
  396.3
  39.63
  3.963
 3895	bbs
  389.5
  38.95
  3.895
 3481	xchar
  348.1
  34.81
  3.481
 3200	tbl
  320
  32
  3.2
 2566	jck
  256.6
  25.66
  2.566
 1989	bsb
  198.9
  19.89
  1.989
 1933	eag
  193.3
  19.33
  1.933
##END