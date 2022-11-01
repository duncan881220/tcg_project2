for i in {1..50}; do
	./threes --total=100000 --block=1000 --limit=1000 --slide=" load=weight.bin save=weight.bin alpha=0.01" | tee -a train.log
	./threes --total=1000 --slide=" load=weight.bin alpha=0" --save="stats.txt"
	tar zcvf weight.$(date +%Y%m%d-%H%M%S).tar.gz weight.bin train.log stats.txt
done