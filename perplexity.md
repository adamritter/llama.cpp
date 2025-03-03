128:
build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 2048  --n-gpu-layers 30
Final estimate: PPL = 22.2950 +/- 1.12682

build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 2048  --n-gpu-layers 30 --override-kv deepseek2.expert_used_count=int:3
PPL=58

1936, 129.30 seconds per pass, 64 MoEs in mem
0 GPU layers: 120 seconds per pass

cmake --build build --config Release -j

orig:
build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 2048  --n-gpu-layers 30 --override-kv deepseek2.expert_used_count=int:3
2.5888...


build/bin/llama-perplexity --model ~/.ollama/models/blobs/sha256-6150cb382311b69f09cc0f9a1b69fc029cbd742b66bb8ec531aa5ecf5c613e93 -f prompt.txt
2.88...  2.68, 164 tps, 11.48 seconds per pass

# same with --n-gpu-layers 30 -> just 55 tps, same perplexity,  37.38 seconds per pass

build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 2048  --n-gpu-layers 30
1.8274

build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 2048  --n-gpu-layers 30 --override-kv deepseek2.expert_used_count=int:10
[1]1.8370,

#



 build/bin/llama-cli --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf --cache-type-k q4_0   --threads 12 --prio 2   --temp 0.6 --ctx-size 8192 --seed 3407 --n-gpu-layers 30 -no-cnv  --prompt "<｜User｜>Find the degree for the given field extension Q(sqrt(2), sqrt(3), sqrt(18)) over Q.. Choices A:0, B:4, C:2, D:6. Just answer A, B, C or D and nothing else.<｜Assistant｜>" --ctx-size 512 --override-kv deepseek2.expert_used_count=int:6

 10 tps 64 MoEs in mem
 10 tps 128 MoEs in mem

 10 tps looks good enough, now let's see how to implement max 1 new expert per token


Next step is to go down from 10 tps to for example 5 tps (half of time is loading data from SSD in that case), but improve perplexity

build/bin/llama-cli --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf --cache-type-k q4_0   --threads 12 --prio 2   --temp 0.6 --ctx-size 8192 --seed 3407 --n-gpu-layers 30 -no-cnv  --prompt "<｜User｜>Find the degree for the given field extension Q(sqrt(2), sqrt(3), sqrt(18)) over Q.. Choices A:0, B:4, C:2, D:6. Just answer A, B, C or D and nothing else.<｜Assistant｜>" --ctx-size 8192 --override-kv deepseek2.expert_used_count=int:3

build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 512  --n-gpu-layers 0 --override-kv deepseek2.expert_used_count=int:3  -- 5.9 (-> ctx size 512 is really bad)
build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 2048  --n-gpu-layers 0 --override-kv deepseek2.expert_used_count=int:3  -- 3.75 with use_lru, 6 min
build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 2048  --n-gpu-layers 0 --override-kv deepseek2.expert_used_count=int:8  -- 2.9  with use_lru 12 min
cmake --build build --config Release -j  && build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 2048  --n-gpu-layers 0 --override-kv deepseek2.expert_used_count=int:8 1.8614 ETA 17.75 minutes
build/bin/llama-perplexity --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf -f prompt.txt  --ctx-size 8192  --n-gpu-layers 0 --override-kv deepseek2.expert_used_count=int:3  -- mem :(


iostat -d -w 1


automating more is needed

 build/bin/llama-cli --model ~/Downloads/deepseek/DeepSeek-R1-UD-IQ1_S-00001-of-00003.gguf --cache-type-k q4_0   --threads 12 --prio 2   --temp 0.6 --ctx-size 8192 --seed 3407 --n-gpu-layers 0 -no-cnv  --prompt "<｜User｜>Find the degree for the given field extension Q(sqrt(2), sqrt(3), sqrt(18)) over Q.. Choices A:0, B:4, C:2, D:6. Just answer A, B, C or D and nothing else.<｜Assistant｜>" --ctx-size 2048 --override-kv deepseek2.expert_used_count=int:8  -- 3 tokens per second (use_lru=true), 3.19 use_lru=false??


 bool loaded = count > 1000 -> 5.7 tps on CPU, perplexity: 3, 12 min
  bool loaded = count > 10000 ->                           2.88, 12 min
  lru: 1.84, 18 min
bool loaded = count % 2 == 0: 1.88, 15 min :)  // count or current_layer0? reproduction looks worse but still OK
bool loaded = count % 4 == 0: 1.84, 21 min :(
bool loaded = count % 4 > 0:  2.02, 14.25 min
bool loaded = count % 3 > 0:  1.95, 15 min
bool loaded = count % 2 > 0: 1.94 now, 18.7 min
Trying currentlayer0 % 2 == 0 again 2.92, 12 min
bool loaded = count % 2 == 0 again: 1.93 18 min

-- Need to compute extra latency - perplexity tradeoff.
-- 546 GB/s memory bandwidth, 128 MoEs in mem, 6.5GB/s SSD bandwidth
-- loading 1 expert / token from 256: 132 GB -> 0.5 GB, can do 10 token/s, not too strong, better every second token
- 8 tokens is 8ms, can transfer 10 tokens in 80ms, should be really easy
- 8 mb / layer, 60 layers -> 500MB / expert

-- cool, now I know that it works, just not the best way toimplement it.

one thing I could do  is to think about simple ops that can implement it. First just in CPU.

- one is getting argmaxes, last LRU state (persistent 128 entries + 1 index), 1 bool (loaded=true/false) and computes new experts to load in LRU position (8 elems). it also should update lru state + 1 index and output a 128 elem ,,to load'' array with mostly 0s, others are index+1 of expert indices to load (1..256)... with maximum 8 non-zero elems (another option is 8 pairs of to load and where to load with optional emtpy elements)

---

create

-DCMAKE_BUILD_TYPE=Debug
