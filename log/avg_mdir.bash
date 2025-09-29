#!/usr/bin/env bash

for f in ./log/nopol.log ./log/pol.log; do
  echo "== $f (A:44444) =="
  awk '/-> 44444/ {
    for(i=1;i<=NF;i++){
      if($i=="delay:"){ sub("us","",$(i+1)); d=$(i+1) }
      if($i=="jitter:"){ sub("us","",$(i+1)); j=$(i+1) }
    }
    if(d!=""){ sumd+=d; if(d>maxd)maxd=d; nd++ }
    if(j!=""){ sumj+=j; if(j>maxj)maxj=j; nj++ }
  } END {
    if(nd) printf("delay avg=%.1f max=%d (n=%d)\n", sumd/nd, maxd, nd);
    if(nj) printf("jitter avg=%.1f max=%d (n=%d)\n", sumj/nj, maxj, nj);
  }' "$f"

  echo "== $f (B:55555) =="
  awk '/-> 55555/ {
    for(i=1;i<=NF;i++){
      if($i=="delay:"){ sub("us","",$(i+1)); d=$(i+1) }
      if($i=="jitter:"){ sub("us","",$(i+1)); j=$(i+1) }
    }
    if(d!=""){ sumd+=d; if(d>maxd)maxd=d; nd++ }
    if(j!=""){ sumj+=j; if(j>maxj)maxj=j; nj++ }
  } END {
    if(nd) printf("delay avg=%.1f max=%d (n=%d)\n", sumd/nd, maxd, nd);
    if(nj) printf("jitter avg=%.1f max=%d (n=%d)\n", sumj/nj, maxj, nj);
  }' "$f"
done
