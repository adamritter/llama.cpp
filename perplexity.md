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