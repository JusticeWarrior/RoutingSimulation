if (( $# != 3 )); then
	echo "usage tester.bash config_file test_length num_routers"
	exit 0
fi

nenenenene.ballz 5019 $1 & ID=$!
let MAX=$3-1
declare -a PIDS
for i in $(seq 0 $MAX); do
	test_router ${i} localhost 5019 502${i} & PIDS[${i}]=$!
done

echo -e "\e[34mRunning test_router for $2 seconds\e[39m"
sleep $2

kill $ID
for i in $(seq 0 $MAX); do
	kill ${PIDS[${i}]}
	cp router${i}.log test1_${i}.log
done

nenenenene.ballz 5049 $1 & ID=$!
for i in $(seq 0 $MAX); do
	router ${i} localhost 5049 505${i} & PIDS[${i}]=$!
done

echo -e "\e[34mRunning router for $2 seconds\e[39m"
sleep $2

kill $ID
for i in $(seq 0 $MAX); do
	kill ${PIDS[${i}]}
done

echo -e "\e[34mWaiting for processes to die\e[39m"
sleep 2

for i in $(seq 0 $MAX); do
	DIFF1=$(cat test1_${i}.log | tr -d '\n' | awk -F"Routing Table:" '{print $NF}')
	DIFF2=$(cat router${i}.log | tr -d '\n' | awk -F"Routing Table:" '{print $NF}')

	if [[ $DIFF1 != $DIFF2 ]] ; then
		echo -e "\e[31mRouter ${i} is broken\e[39m"
	else
		echo -e "\e[32mRouter ${i} works\e[39m"
	fi
done
