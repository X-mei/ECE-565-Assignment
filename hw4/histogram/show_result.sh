make
path='./result_report_record'
declare -a types=("histo" "histo_locks" "histo_atomic" "histo_creative")
declare -a num_threads=(2 4 8)
for num in "${num_threads[@]}"
do
    echo -e "\n-------- OMP_NUM_THREADS = $num --------"
    for i in "${types[@]}"
    do
        echo -e "\n*** $i ***"
        outpath="$path/${i}_${num}_large.out"
        diff validation-large.out $outpath
    done
done