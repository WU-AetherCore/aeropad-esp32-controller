"""Original puzzles: reverse-push BFS from goals proves minimum push counts.

No third-party maps or artwork are copied. Run once to regenerate the fixed pack.
Player states are normalized to their reachable component; all solved player
components seed the BFS, so the stored distance is the global minimum pushes.
"""
import random,json,time,sys
from collections import deque
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
W=H=8;N=W*H
def neighbours(i):
 return tuple(j for j in (i-W,i+1,i+W,i-1) if 0<=j<N and abs(j%W-i%W)+abs(j//W-i//W)==1)
ADJ=[neighbours(i) for i in range(N)]
NOT_LEFT=sum(1<<i for i in range(N) if i%W!=0)
NOT_RIGHT=sum(1<<i for i in range(N) if i%W!=W-1)
def region(start,boxes,floor):
 seen=1<<start;free=floor&~boxes
 while True:
  expanded=seen|(((seen<<W)|(seen>>W)|((seen&NOT_RIGHT)<<1)|((seen&NOT_LEFT)>>1))&free)
  if expanded==seen:return seen
  seen=expanded
def low(mask):return (mask&-mask).bit_length()-1
def enumerate_reverse(floor,goals,target,cap=180000):
 boxes=sum(1<<b for b in goals);available=floor&~boxes;roots=[]
 while available:
  r=region(low(available),boxes,floor);roots.append((boxes,low(r)));available&=~r
 q=deque(roots);parents={s:None for s in roots};depth={s:0 for s in roots}
 while q:
  state=q.popleft();bs,p=state;d=depth[state]
  if d>=target:return state,parents,d
  reach=region(p,bs,floor);rest=bs
  while rest:
   b=low(rest);rest&=rest-1
   for near in ADJ[b]:
    beyond=near+(near-b)
    if beyond not in ADJ[near]:continue
    if reach&(1<<near) and floor&(1<<beyond) and not bs&(1<<beyond):
     newbs=(bs^(1<<b))|(1<<near);newp=low(region(beyond,newbs,floor));s=(newbs,newp)
     if s not in parents:
      parents[s]=(state,b,near,beyond);depth[s]=d+1;q.append(s)
  if len(parents)>cap:return state,parents,d
 return state,parents,d
def walk(start,dest,boxes,floor):
 q=deque([start]);paths={start:''}
 while q:
  i=q.popleft()
  if i==dest:return paths[i]
  for j in ADJ[i]:
   if floor&(1<<j) and not boxes&(1<<j) and j not in paths:
    letter='U' if j-i==-W else 'D' if j-i==W else 'R' if j-i==1 else 'L';paths[j]=paths[i]+letter;q.append(j)
 raise RuntimeError('No walking path')
def build():
 (ROOT/'logs').mkdir(exist_ok=True)
 rng=random.Random(1707607);chosen=[];targets=[8,12,16,20,25,30,36,43,51,53];started=time.time()
 checkpoint=ROOT/'logs/sokoban_generation_checkpoint.json'
 if '--resume' in sys.argv and checkpoint.exists():
  saved=json.loads(checkpoint.read_text());chosen=saved['chosen']
  def tuples(v):return tuple(tuples(i) for i in v) if isinstance(v,list) else v
  rng.setstate(tuples(saved['rng']))
 for number,target in enumerate(targets):
  if number<len(chosen):continue
  attempts=0;best=0
  while True:
   attempts+=1
   floors=[y*W+x for y in range(1,7) for x in range(1,7)]
   walls=set(rng.sample(floors,rng.randrange(7,14) if number>=6 else rng.randrange(5,10)))
   base=rng.choice(chosen[-2:])['rows'] if number==9 and attempts%5!=0 else chosen[-1]['rows'] if number==8 and attempts%5!=0 else None
   if base:
    walls={i for i in floors if base[i//W][i%W]=='#'}
    if attempts%3==0:
     candidates=[i for i in floors if base[i//W][i%W] not in '.*+']
     for cell in rng.sample(candidates,3 if number==9 else 1):
      if cell in walls:walls.remove(cell)
      else:walls.add(cell)
   floor=sum(1<<i for i in floors if i not in walls)
   if region(low(floor),0,floor)!=floor:continue
   options=[i for i in floors if floor&(1<<i) and len([j for j in ADJ[i] if floor&(1<<j)])>=2]
   goalcount=2 if number<2 else 3 if number<5 else number-1
   if base:
    inherited=[i for i in options if base[i//W][i%W] in '.*+']
    goals=tuple(inherited+rng.sample([i for i in options if i not in inherited],goalcount-len(inherited)))
   elif number>=6 and rng.random()<.5:
    corner=rng.choice((9,14,49,54));options.sort(key=lambda i:abs(i%8-corner%8)+abs(i//8-corner//8)+rng.random()*4);goals=tuple(options[:goalcount])
   else:goals=tuple(rng.sample(options,goalcount))
   result=enumerate_reverse(floor,goals,target)
   if result is None:continue
   (boxes,p),parents,minimum=result;best=max(best,minimum)
   if attempts%100==0:print(f'Search level {number+1}: candidates {attempts}, best depth {best}, elapsed {time.time()-started:.1f}s',flush=True)
   if minimum<target:continue
   state=(boxes,p);player=p;solution='';bs=boxes
   while parents[state] is not None:
    older,oldbox,near,beyond=parents[state]
    solution+=walk(player,beyond,bs,floor)
    delta=oldbox-near;solution+='U' if delta==-W else 'D' if delta==W else 'R' if delta==1 else 'L'
    player=near;bs=(bs^(1<<near))|(1<<oldbox);state=older
   rows=[]
   for y in range(H):
    row=''
    for x in range(W):
     i=y*W+x;row+='#' if not floor&(1<<i) else '*' if boxes&(1<<i) and i in goals else '$' if boxes&(1<<i) else '+' if i==p and i in goals else '@' if i==p else '.' if i in goals else ' '
    rows.append(row)
   # Early levels start with most goals empty. Advanced levels may require
   # moving already-placed boxes out of the way before putting them back.
   if number<8 and sum(1 for g in goals if boxes&(1<<g))>goalcount//2:continue
   chosen.append(dict(level=number+1,rows=rows,minimum_pushes=minimum,solution=solution,boxes=goalcount,states=len(parents)))
   checkpoint.write_text(json.dumps(dict(chosen=chosen,rng=rng.getstate())))
   print(f'Level {number+1}: minimum pushes {minimum}, boxes {goalcount}, solution moves {len(solution)}, candidates {attempts}, elapsed {time.time()-started:.1f}s',flush=True);break
 text=['#pragma once','// Original WU-AetherCore pack; reverse BFS certified minimum pushes.','namespace LocalGames {','static const char* const sokobanMaps[] = {']
 for l in chosen:text+=['    '+''.join(json.dumps(row) for row in l['rows'])+',']
 text+=['};','static const unsigned char sokobanMinimumPushes[] = {'+','.join(str(l['minimum_pushes']) for l in chosen)+'};','}']
 (ROOT/'src/SokobanLevels.h').write_text('\n'.join(text)+'\n')
 (ROOT/'tools/sokoban_solutions.h').write_text('#pragma once\nstatic const char* const sokobanSolutions[] = {\n'+''.join('    '+json.dumps(l['solution'])+',\n' for l in chosen)+'};\n')
 (ROOT/'docs/sokoban_levels.json').write_text(json.dumps(dict(width=W,height=H,method='Reverse push BFS, all solved player components',levels=chosen),indent=2)+'\n')
if __name__=='__main__':build()
