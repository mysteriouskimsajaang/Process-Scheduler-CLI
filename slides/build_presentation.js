// Editable technical report slides for CSOPESY Major Output 1.
// Requires npm package pptxgenjs (used to generate the provided PPTX).
const pptxgen = require('pptxgenjs');
const pptx = new pptxgen();
pptx.layout = 'LAYOUT_WIDE';
pptx.author = 'CSOPESY Project Group';
pptx.subject = 'Process Scheduler and Command-line Emulator';
pptx.title = 'CSOPESY - Process Scheduler and CLI';
pptx.company = 'Academic technical report';
pptx.lang = 'en-US';
pptx.theme = {
  headFontFace: 'Aptos Display', bodyFontFace: 'Aptos', lang: 'en-US'
};
const W = 13.333, H = 7.5;
const C={bg:'0B1322',card:'142239',card2:'1B2C45',white:'F7FBFF',muted:'ACBED5',teal:'38E0C1',blue:'7CAEFF',amber:'FFC66B',line:'32445D',gray:'718399',green:'71EFA7',red:'FF8C93'};
const ST=pptx.ShapeType;
const tx=(s,t,x,y,w,h,o={})=>s.addText(t,{x,y,w,h,fontFace:'Aptos',fontSize:15,color:C.white,breakLine:false,margin:0,bold:false,fit:'shrink',...o});
const shape=(s,x,y,w,h,fill=C.card,r=0.17,line=C.card)=>s.addShape(r ? ST.roundRect : ST.rect,{x,y,w,h,rectRadius:r, radius:r,fill:{color:fill},line:{color:line,transparency:line===fill?100:0,width:0.8}});
const line=(s,x,y,w,h,color=C.line,width=1)=>s.addShape(ST.line,{x,y,w,h,line:{color,width}});
const chip=(s,t,x,y,w,fill=C.card2,color=C.teal)=>{shape(s,x,y,w,.32,fill,.12);tx(s,t,x+.12,y+.07,w-.24,.16,{fontSize:9.1,bold:true,color,align:'center'});};
const header=(s,kicker,title,n)=>{
  s.background={color:C.bg};
  tx(s,kicker.toUpperCase(),.62,.38,11,.21,{fontSize:10,bold:true,charSpacing:2.2,color:C.teal});
  tx(s,title,.62,.83,12.1,.59,{fontSize:30,bold:true});
  line(s,.62,1.54,12.07,0,C.line,1);
  tx(s,'CSOPESY  /  PROCESS SCHEDULER & CLI',.62,7.17,7,.12,{fontSize:8.2,color:C.gray,charSpacing:1.1});
  tx(s,String(n).padStart(2,'0')+'  /  10',11.6,7.16,1.06,.15,{fontSize:9,color:C.gray,align:'right'});
};
const card=(s,x,y,w,h,title,body,accent=C.teal)=>{
 shape(s,x,y,w,h,C.card,.18);shape(s,x,y,.065,h,accent,0);tx(s,title,x+.27,y+.24,w-.54,.36,{fontSize:18,bold:true});
 tx(s,body,x+.27,y+.73,w-.54,h-.92,{fontSize:13,color:C.muted,breakLine:false,valign:'top'});
};
const mono=(s,t,x,y,w,h,sz=12)=>{
 shape(s,x,y,w,h,'08101D',.18);
 tx(s,t,x+.24,y+.2,w-.47,h-.35,{fontFace:'Liberation Mono',fontSize:sz,color:'D7E8FF',breakLine:false,valign:'top'});
};
const arrow=(s,x,y,w=.63,color=C.teal)=>{
  line(s,x,y,w,0,color,2.5);s.addShape(ST.chevron,{x:x+w-.12,y:y-.11,w:.23,h:.22,line:{color,transparency:100},fill:{color}});
};
const bulletText=(s,arr,x,y,w,step=.6,fontSize=15)=>arr.forEach((p,i)=>{
 shape(s,x,y+i*step+.08,.085,.085,C.teal);
 tx(s,p,x+.23,y+i*step,w-.26,.43,{fontSize,color:C.white});
});

// 1 Cover.
{
 const s=pptx.addSlide();s.background={color:C.bg};
 shape(s,.58,.74,.08,5.8,C.teal);
 tx(s,'MAJOR OUTPUT 01',1.05,1.12,9,.33,{fontSize:14,color:C.teal,bold:true,charSpacing:2});
 tx(s,'PROCESS\nSCHEDULER',1.02,1.85,10.8,1.77,{fontSize:54,bold:true,breakLine:false,lineSpacingMultiple:.89});
 tx(s,'& COMMAND-LINE EMULATOR',1.07,3.97,10.4,.47,{fontSize:24,color:C.blue,bold:true});
 tx(s,'C++17  ·  FCFS / ROUND ROBIN  ·  VIRTUAL MULTI-CORE CPU',1.09,4.77,11,.38,{fontSize:14,color:C.muted,charSpacing:.6});
 chip(s,'IMPLEMENTATION & TECHNICAL REPORT',1.08,5.53,3.6);
 tx(s,'Group members: [Add names]     Section: [Add section]',1.07,6.65,10,.3,{fontSize:12,color:C.muted});
}
// 2 Contract.
{
 const s=pptx.addSlide();header(s,'Scope','Assignment requirements covered',2);
 card(s,.63,1.88,5.95,4.49,'Command-line interface',
  'initialize  ·  exit\nscreen -s / -r / -ls\nscheduler-start  ·  scheduler-stop\nreport-util  →  csopesy-log.txt\n\nInitialization required before other commands.',C.teal);
 card(s,6.77,1.88,5.94,4.49,'OS scheduler behavior',
  'FCFS and time-sliced Round Robin\n1–128 simulated CPU cores\nReady, running, sleeping, finished states\nProcess-specific PRINT logs\nRandomized typed instructions and nested FOR\nConfiguration from config.txt',C.blue);
 tx(s,'Run-time options come from a space-separated config file; the binary does not need recompilation for parameter changes.',.71,6.57,11.8,.28,{fontSize:12,color:C.muted,italic:true});
}
// 3 Architecture.
{
 const s=pptx.addSlide();header(s,'Design','Architecture & ownership',3);
 const boxes=[
  {x:.8,t:'TERMINAL CLI',b:'User commands'},
  {x:4.01,t:'COMMAND PARSER',b:'Main / screen modes'},
  {x:7.22,t:'SCHEDULER',b:'Virtual CPU ticks'},
  {x:10.43,t:'PROCESS STORE',b:'Programs + logs'}
 ];
 boxes.forEach((b,i)=>{shape(s,b.x,2.17,2.52,1.29,C.card,.13);tx(s,b.t,b.x+.16,2.52,2.2,.26,{fontSize:15.6,bold:true,color:i===2?C.teal:C.white,align:'center'});tx(s,b.b,b.x+.15,2.99,2.22,.24,{fontSize:11,color:C.muted,align:'center'});if(i<3) arrow(s,b.x+2.56,2.81,.6)});
 shape(s,3.1,4.16,7.22,1.43,C.card2,.16);
 tx(s,'BACKGROUND SIMULATION THREAD',3.37,4.46,6.63,.26,{fontSize:19,bold:true,align:'center',color:C.teal});
 tx(s,'20 ms tick  •  mutex-guarded state  •  process scheduling continues while user types',3.38,4.96,6.65,.35,{fontSize:13,color:C.muted,align:'center'});
 line(s,8.5,3.52,0,.64,C.teal,2.5);
 tx(s,'One scheduling worker simulates many virtual cores; it does not create one operating-system thread per process.',1.06,6.2,11.3,.6,{fontSize:14,color:C.muted,align:'center'});
}
// 4 CLI.
{
 const s=pptx.addSlide();header(s,'Command recognition','Two terminal modes',4);
 mono(s,'root:\\> initialize\nInitialized 4 CPU(s), round robin scheduler.\nroot:\\> screen -s demo\n\nProcess name: demo\nID: 1\nStatus: READY\nCurrent instruction line: 0\nLines of code: 21\n\nscreen:demo> process-smi\nscreen:demo> exit\nroot:\\> report-util',.67,1.91,7.0,4.97,14.1);
 card(s,7.92,1.93,4.73,2.23,'Main menu','Rejects other commands before initialize. Parses screen, scheduler and report commands.',C.teal);
 card(s,7.92,4.36,4.73,2.48,'Attached screen','Only process-smi (refresh status and logs) and exit (detach). The process keeps executing.',C.amber);
}
// 5 Processes.
{
 const s=pptx.addSlide();header(s,'Process model','Process data & state transitions',5);
 const fields=[['Identity','Name, process ID, creation time'],['Execution','Program counter, total instructions'],['Scheduling','Status, last core, time slice, delay'],['Environment','uint16 variables and PRINT logs'],['Timing','Sleep wake tick and finished time']];
 fields.forEach((row,i)=>{shape(s,.73,1.89+i*.87,5.24,.7,i%2?C.card2:C.card,.11);tx(s,row[0],.95,2.09+i*.87,1.54,.23,{fontSize:13.2,color:C.teal,bold:true});tx(s,row[1],2.38,2.09+i*.87,3.31,.25,{fontSize:12.3,color:C.white});});
 const nodes=[{t:'READY',x:7.48,y:2.14,c:C.blue},{t:'RUNNING',x:10.09,y:2.14,c:C.teal},{t:'SLEEPING',x:7.48,y:4.62,c:C.amber},{t:'FINISHED',x:10.09,y:4.62,c:C.green}];
 nodes.forEach(n=>{shape(s,n.x,n.y,2.07,.75,C.card,.15,n.c);tx(s,n.t,n.x,n.y+.23,2.07,.23,{fontSize:14,bold:true,align:'center',color:n.c});});
 arrow(s,9.55,2.51,.5,C.blue);
 line(s,10.99,2.89,0,1.73,C.teal,2.4);tx(s,'complete',11.1,3.49,1.3,.22,{fontSize:11,color:C.muted});
 line(s,10.09,2.89,-.78,1.73,C.amber,2.4);tx(s,'SLEEP(X)',8.89,3.62,1.24,.25,{fontSize:11,color:C.amber});
 line(s,7.47,4.99,-.35,-1.91,C.blue,2.4);tx(s,'wake / enqueue',6.08,3.52,1.5,.4,{fontSize:11,color:C.muted});
 tx(s,'RR timeslice expiry: RUNNING → READY',7.27,6.31,5.03,.36,{fontSize:13.2,color:C.muted,align:'center'});
}
// 6 Scheduling.
{
 const s=pptx.addSlide();header(s,'CPU scheduling','FCFS vs Round Robin',6);
 tx(s,'FCFS  /  NONPREEMPTIVE',.81,1.95,5.8,.38,{fontSize:17,bold:true,color:C.teal});
 tx(s,'A process holds a core until finish or SLEEP.',.81,2.43,5.5,.24,{fontSize:13,color:C.muted});
 const x=.85,y=2.98, cell=.67;
 ['A','A','A','A','B','B','B','B'].forEach((v,i)=>{shape(s,x+i*.72,y,cell,.61,v==='A'?C.blue:C.amber,.07);tx(s,v,x+i*.72,y+.13,cell,.3,{fontSize:16,bold:true,align:'center',color:C.bg});});
 tx(s,'RR  /  QUANTUM = 2 TICKS',.81,4.28,5.8,.38,{fontSize:17,bold:true,color:C.teal});
 tx(s,'Processes are re-queued after consuming each time slice.',.81,4.77,5.83,.23,{fontSize:13,color:C.muted});
 ['A','A','B','B','A','A','B','B'].forEach((v,i)=>{shape(s,x+i*.72,5.34,cell,.61,v==='A'?C.blue:C.amber,.07);tx(s,v,x+i*.72,5.46,cell,.3,{fontSize:16,bold:true,align:'center',color:C.bg});});
 card(s,7.25,2.12,5.35,3.93,'Multi-core dispatch','Each simulated core holds at most one RUNNING process. Available cores take the next READY process. SLEEP yields a core. RR preemption returns unfinished processes to the queue.\n\nCPU utilization = running cores / configured cores × 100%.',C.blue);
 chip(s,'PROCESS A',.86,6.3,1.32,C.blue,C.bg);chip(s,'PROCESS B',2.31,6.3,1.32,C.amber,C.bg);
}
// 7 Tick execution.
{
 const s=pptx.addSlide();header(s,'Real-time engine','What happens during each CPU tick?',7);
 const steps=[['01','INCREMENT','Increase virtual tick counter'],['02','GENERATE','Add p1 / p2 / ... on batch interval'],['03','WAKE','Move expired sleepers to READY'],['04','DISPATCH','Assign free CPUs from ready queue'],['05','EXECUTE','Advance one instruction or wait tick'],['06','REQUEUE','Finish, sleep or RR preempt']];
 steps.forEach((it,i)=>{
   const col=i%3,row=Math.floor(i/3),x=.72+col*4.19,y=1.97+row*2.16;
   shape(s,x,y,3.88,1.76,C.card,.15);
   tx(s,it[0],x+.19,y+.23,.58,.38,{fontSize:25,bold:true,color:C.teal});
   tx(s,it[1],x+.84,y+.26,2.76,.28,{fontSize:17,bold:true});
   tx(s,it[2],x+.27,y+.87,3.34,.67,{fontSize:13,color:C.muted});
 });
 chip(s,'1 simulated tick = 20 ms in this build',3.86,6.54,5.59,C.card2,C.blue);
}
// 8 Instruction handling.
{
 const s=pptx.addSlide();header(s,'Interpreter','Instruction set and variables',8);
 const rows=[
 ['PRINT','"Hello world from <name>!"; supports variable output'],
 ['DECLARE','Initialize a named uint16 value'],
 ['ADD','target = operand1 + operand2; clamp to 65535'],
 ['SUBTRACT','target = operand1 − operand2; clamp to 0'],
 ['SLEEP','Release CPU for X simulated ticks'],
 ['FOR','Nested instruction arrays, up to 3 levels deep']
 ];
 rows.forEach((row,i)=>{const yy=1.97+i*.74;shape(s,.75,yy,11.8,.63,i%2?C.card2:C.card,.1);tx(s,row[0],1.02,yy+.18,2.03,.25,{fontSize:15,bold:true,color:C.teal});tx(s,row[1],3.18,yy+.16,8.78,.28,{fontSize:13.5,color:C.white});});
 tx(s,'Before execution, FOR nodes are recursively expanded into executable leaf instructions; process instruction counts are exact.',1.06,6.65,11.19,.3,{fontSize:12.5,color:C.muted,italic:true});
}
// 9 Configuration and reporting.
{
 const s=pptx.addSlide();header(s,'Observability','Configuration & utilization output',9);
 mono(s,'config.txt\n\nnum-cpu 4\nscheduler "rr"\nquantum-cycles 5\nbatch-process-freq 1\nmin-ins 1000\nmax-ins 2000\ndelays-per-exec 0',.76,2.05,5.47,4.71,15);
 mono(s,'screen -ls / report-util\n\nCPU utilization: 50.0%\nCores used: 2\nCores available: 2\nCPU tick: 120\n---------------------\nRunning processes:\n  p1   Core: 0  10 / 40\n  p2   Core: 1  12 / 40\nFinished processes:\n  demo Finished 20 / 20',6.46,2.05,6.1,4.71,13);
 chip(s,'Sample report above is an illustrative layout, not measured output.',3.27,6.95,6.86,C.card2,C.muted);
}
// 10 QA and submission.
{
 const s=pptx.addSlide();header(s,'Verification','Tests and submission package',10);
 card(s,.72,1.91,5.95,4.6,'Verified test coverage',
  'Config validation and initialization\nFCFS, RR time-slice preemption, 2 cores\nSLEEP relinquishing the CPU\nBusy-wait between instructions\nuint16 clamping and undeclared variables\nNested FOR, PRINT logs, instruction counts\nGenerator start/stop and CLI smoke test',C.green);
 card(s,6.88,1.91,5.75,4.6,'Submission checklist',
  'SOURCE  ·  C++ files, CMake and scripts\nREADME  ·  Build/run instructions + entry point\nPPT  ·  This editable technical report\nMP4  ·  Record actual instructor quiz cases\n\nBefore submission: replace group/section placeholders, check instructor configuration and capture real demonstrations.',C.amber);
 tx(s,'Demo script and automated tests are included, but the instructor-specific MP4 recordings must be made during the actual evaluation.',.92,6.64,11.5,.42,{fontSize:12,color:C.muted,align:'center'});
}

pptx.writeFile({fileName:require('path').join(__dirname,'..','Technical_Report.pptx')});
