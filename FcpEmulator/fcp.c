/* $Header: /home/qiana/Repository/FcpEmulator/fcp.c,v 1.1 1999/07/01 07:15:09 bill Exp $ */
/*
**  fcp.c  -  emulator startup
*/


#include	<stdio.h>
#ifdef	SUNOS5d3
#include	<sys/fcntl.h>
#endif
#include	<string.h>
#include	<ctype.h>
#include	<sys/file.h>

#include	"fcp.h"
#include	"codes.h"
#include	"global.h"
#include	"macros.h"

char	*ProgName = (char *) Null;	/* Program Name for link kernel */
FILE	*OutFile;			/* Output File */
FILE	*DbgFile;			/* Debugging File */


int	SpecRsrv = False;
int	RsrvSize = 0;
int	SpecLink = False;
int	LinkSize = 0;
int	SpecHeap = False;
int	HeapSize = 0;
int	SpecTrls = False;
int	TrlsSize = 0;
int	SpecTbls = False;
int	TblsSize = 0;
int	SpecTime = False;
int	SpecDevsTime = False;
char	*FileName = (char *) Null;
int	BootType = WarmBoot;
char	*Parent_Addr = (char *) Null;

#ifdef	DEBUG
int	ReductionsDebug = 0;
int	GCDebug = 0;
#endif

fcp(argc, argv)
     int	argc;
     char	*argv[];
{
  heapT Prcdr;

  register int	Fd;
  register char *PMem, *MemEnd;

  char *Base;
  int Size;

  extern char *malloc();

  DbgFile = stderr;
  OutFile = stdout;

  HighQ = 0;
  TempSus = False;

  TimeSlice = 0;
  TS = 0;
  DevsTime = 0;
  McnOutM = 0xffffffff;

  TimeDelta = 0;
  
  Creations = 0;
  Suspensions = 0;
  Activations = 0;
  Failures = 0;
  Losses = 0;
  Switches = 0;
  Reductions = 0;
  Terminations = 0;
  CpuTime = 0;
  GCTime = 0;
  Collections = 0;
  FreedAverage = 0;
  CopiedAverage = 0;
  GCMinFlt = 0;
  GCMajFlt = 0;
  
#ifdef	DEBUG
  Debug_Process = False;
  Debug_Clause = False;
  Debug_Guard = False;
  Debug_Outargs = False;
  Debug_Activation = False;
  Debug = False;
  Debug_Unset = False;
  Debug_Buffered = False;
#endif

  StrOrder[0] = 0x0;
  StrOrder[1] = 0x1;
  StrOrder[2] = 0x3;
  StrOrder[3] = 0x4;
  StrOrder[4] = 0x2;

  if (!get_user_args(argc, argv)) {
    return(False);
  }
  if (BootType != WarmBoot) {
    PMem = malloc(RsrvSize+LinkSize+HeapSize);
    if (PMem == NULL) {
      fprintf(DbgFile, "fcp: not enough core for link/heap area\n");
      return(False);
    }
    LinkBase = (linkP) (PMem+RsrvSize);
    LinkEnd = LinkBase + (LinkSize / sizeof(linkT));
    HeapBase = (heapP) LinkEnd;
    HeapEnd = HeapBase + (HeapSize / sizeof(heapT));
    if (((((int) HeapBase) & HOByteMask) != HOByte) ||
	((((int) HeapBase) & HOByteMask) != HOByte)) {
      fprintf(DbgFile, "fcp: Unexpected address format.\n");
      return(False);
    }
    HeapStart = HeapBase;
    HP = HeapStart;
    CurHeapLimit = HeapEnd;
    PMem = malloc(TrlsSize+TblsSize);
    if (PMem == NULL) {
      fprintf(DbgFile, "fcp: not enough core for trails/tables\n");
      return(False);
    }
    AsgnTrl = (trailP) PMem;
    ChngTrl = ((trailP) (PMem + TrlsSize)) - 1;
    SusTbl = (heapP) (PMem + TrlsSize);
    ErrTbl = ((heapP) (PMem + TrlsSize + TblsSize)) - 1;
    asgn_trl_reset();
    chng_trl_reset();
    sus_tbl_reset();
    err_tbl_reset();
    if (!produce_constants()) {
      fprintf(DbgFile, "fcp: not enough space for constants\n");
      return(False);
    }
    HeapStart = HP;
    QF = HQB = QB = Nil;
    SQF = SQB = Nil;
    nil_FLs();
    CP = Nil;
    CurHeap = HeapStart;
    CurHeapEnd = HeapStart + (HeapEnd - HeapStart)/2;
    CurHeapLimit = CurHeapEnd - Heap_TH;
    OtherHeap = CurHeapEnd;
    OtherHeapEnd = HeapEnd;
    LP = LinkBase;
    HP = CurHeap;
    HB = HP;
  }
  switch (BootType) {
  case EmulationBoot:
    /*  startup by reading module into heap. */
    Fd = open(FileName, O_RDONLY);
    if (Fd < 0) {
      fprintf(DbgFile, "fcp: cannot open file %s\n", FileName);
      return(False);
    }
    read(Fd, HP, 2*sizeof(heapT)); /* string header */
#ifdef	ULTRIX
    cnv_w(HP);
    cnv_w(HP+1);
#endif
    if (!IsStr(*HP) || (Str_Type(HP) != MdlType) ||
	(read(Fd, (HP+2), Str_Length(HP)) != Str_Length(HP))) {
      fprintf(DbgFile, "fcp: file %s is not in correct format\n", FileName);
      close(Fd);
      return(False);
    }
    close(Fd);
#ifdef	ULTRIX
    cnv_w(HP);
    cnv_w(HP+1);
    if (!imp_convert(HP)) {
      fprintf(DbgFile, "fcp: file %s is not in correct format\n", FileName);
      return(False);
    }
#endif
    HP += 2 + Str_Words(HP);
    HB = HP;
    Prcdr = Ref_Word((CurHeap+MdlHdrWords));
    break;
  case NativeBoot:
    /* startup by linking module into link area */
    {
      register heapP PString = produce_string(FileName);
      register heapP PList;

      if (PString == Nil) {
	fprintf(DbgFile, "fcp: not enough heap space for linking\n");
      }
      PList = HP;
      *HP++ = L_Ref_Word(PString);
      *HP++ = Word(0, NilTag);
      if (!do_link(PList)) {
	fprintf(DbgFile, "fcp: can not dynamically link file %s\n", FileName);
	return(False);
      }
      Prcdr = KOutA;
    }
    HP = HB;
    break;
  case WarmBoot:
    Fd = open(FileName, O_RDONLY);
    if (Fd < 0) {
      fprintf(DbgFile, "fcp: cannot open file %s\n", FileName);
      return(False);
    }
    read(Fd, &Size, sizeof(Size)); /* Saved RsrvSize */
    if (!SpecRsrv) {
      RsrvSize = Size;
    }
    read(Fd, &Size, sizeof(Size)); /* Saved LinkSize */
    if (SpecLink && (LinkSize != Size)) {
      fprintf(DbgFile, "fcp: can not change saved link size\n");
      close(Fd);
      return(False);
    }
    read(Fd, &Size, sizeof(Size)); /* Saved HeapSize */
    if (!SpecHeap) {
      HeapSize = Size;
    }
    read(Fd, &Size, sizeof(Size)); /* Saved TrlsSize */
    if (!SpecTrls) {
      TrlsSize = Size;
    }
    read(Fd, &Size, sizeof(Size)); /* Saved TblsSize */
    if (!SpecTbls) {
      TblsSize = Size;
    }
    PMem = malloc(RsrvSize+LinkSize+HeapSize);
    if (PMem == NULL) {
      fprintf(DbgFile, "fcp: not enough core for link/heap area\n");
      close(Fd);
      return(False);
    }
    MemEnd = PMem + (RsrvSize+LinkSize+HeapSize);
    /* Load Link */
    read(Fd, &Base, sizeof(Base)); /* Saved LinkBase */
    if ((Base < PMem) || ((Base+LinkSize) > MemEnd)) {
      fprintf(DbgFile, "fcp: restoring link to different location\n");
      close(Fd);
      return(False);
    }
    LinkBase = (linkP) Base;
    LinkEnd = LinkBase + (LinkSize / sizeof(linkT));
    read(Fd, &Size, sizeof(Size)); /* Size of Saved Link */
    if (read(Fd, LinkBase, Size) != Size) {
      fprintf(DbgFile, "fcp: file %s is not in correct format\n", FileName);
      close(Fd);
      return(False);
    }
    /* Load Heap */
    read(Fd, &Base, sizeof(Base)); /* Saved HeapBase */
    if ((Base < ((char *) LinkEnd)) || ((Base+HeapSize) > MemEnd)) {
      fprintf(DbgFile, "fcp: restoring heap to different location\n");
      close(Fd);
      return(False);
    }
    HeapBase = (heapP) Base;
    HeapEnd = HeapBase + (HeapSize / sizeof(heapT));
    read(Fd, &HeapStart, sizeof(HeapStart));  /* Saved HeapStart */
    read(Fd, &CurHeap, sizeof(CurHeap));  /* Saved CurHeap */
    CurHeapEnd = CurHeap + (HeapEnd - CurHeap)/2;
    CurHeapLimit = CurHeapEnd - Heap_TH;
    OtherHeap = CurHeapEnd;
    OtherHeapEnd = HeapEnd;
    read(Fd, &Size, sizeof(Size)); /* Size of actually Saved Heap */
    if (HeapBase + (Size / sizeof(heapT)) > CurHeapLimit) {
      fprintf(DbgFile, "fcp: heap smaller then saved\n");
      close(Fd);
      return(False);
    }
    if (read(Fd, HeapBase, Size) != Size) {  /* read the saved heap */
      fprintf(DbgFile, "fcp: file %s is not in correct format\n", FileName);
      close(Fd);
      return(False);
    }
    HP = HeapBase + (Size/sizeof(heapT));
    PMem = malloc(TrlsSize+TblsSize);
    if (PMem == NULL) {
      fprintf(DbgFile, "fcp: not enough core for trails/tables\n");
      return(False);
    }
    AsgnTrl = (trailP) PMem;
    ChngTrl = ((trailP) (PMem + TrlsSize)) - 1;
    SusTbl = (heapP) (PMem + TrlsSize);
    ErrTbl = ((heapP) (PMem + TrlsSize + TblsSize)) - 1;
    asgn_trl_reset();
    chng_trl_reset();
    sus_tbl_reset();
    err_tbl_reset();
    rstr_vars(); /* Load Vars */
#ifdef	DLVS
    dlvsRestore(Fd);
#endif
    close(Fd);
    break;
  }
  fcp_init_io();
  init_stats();
  start_time();
  emulate(Prcdr, W);
  stop_time();
  prs_print();
  print_stats("Statistics");
  fcp_reset_io();
}

int
get_user_args(argc, argv)
     int	argc;
     char	*argv[];
{
  char	*s;

  SpecRsrv = False;
  RsrvSize = Def_RsrvSize;
  SpecLink = False;
  LinkSize = Def_LinkSize;
  SpecHeap = False;
  HeapSize = Def_HeapSize;
  SpecTrls = False;
  TrlsSize = Def_TrlsSize;
  SpecTbls = False;
  TblsSize = Def_TblsSize;
  SpecTime = False;
  TimeSlice = Def_TimeSlice;
  SpecDevsTime = False;
  DevsTime = Def_DevsTime;

  ProgName = *argv;
  while (--argc > 0 && (*++argv)[0] == '-') {
    for (s = argv[0]+1; *s != '\0'; ) {
      switch (*s) {
#ifdef	DEBUG
      case 'p':
	Debug_Process = True;
	s++;
	break;
      case 'c': 
	Debug_Clause = True;
	s++;
	break;
      case 'g':
	Debug_Guard = True;
	s++;
	break;
      case 'o':
	Debug_Outargs = True;
	s++;
	break;
      case 'a':
	Debug_Activation = True;
	s++;
	break;
      case 'u':
	Debug_Unset = True;
	s++;
	break;
      case 'v':
	s++;
	ReductionsDebug = 0;
	while(isdigit(*s)) {
	  ReductionsDebug = ReductionsDebug*10 + *s++ - '0';
	}
	break;
      case 'B':
	Debug_Buffered = True;
	s++;
	break;
      case 'G':
	s++;
	GCDebug = 0;
	while(isdigit(*s)) {
	  GCDebug = GCDebug*10 + *s++ - '0';
	}
	break;
#endif
      case 'P':
	Parent_Addr = (s+1);
	s += strlen(s);
	break;
      case 'r':
	s++;
	RsrvSize = 0;
	while(isdigit(*s)) {
	  RsrvSize = RsrvSize*10 + *s++ - '0';
	}
	SpecRsrv = True;
	break;
      case 'l':
	s++;
	LinkSize = 0;
	while(isdigit(*s)) {
	  LinkSize = LinkSize*10 + *s++ - '0';
	}
	SpecLink = True;
	LinkSize = LinkSize * 1024;
	break;
      case 'h':
	s++;
	HeapSize = 0;
	while(isdigit(*s)) {
	  HeapSize = HeapSize*10 + *s++ - '0';
	}
	SpecHeap = True;
	HeapSize = HeapSize * 1024 * 2;
	break;
      case 't':
	s++;
	if (isdigit(*s)) {
	  TimeSlice = 0;
	  while(isdigit(*s)) {
	    TimeSlice = TimeSlice*10 + *s++ - '0';
	  }
	  SpecTime = True;
	}
	else {
	  switch (*s) {
	  case 'r':
	    s++;
	    TrlsSize = 0;
	    while(isdigit(*s)) {
	      TrlsSize = TrlsSize*10 + *s++ - '0';
	    }
	    SpecTrls = True;
	    TrlsSize = TrlsSize * sizeof(trailT);
	    break;
	  case 'b':
	    s++;
	    TblsSize = 0;
	    while(isdigit(*s)) {
	      TblsSize = TblsSize*10 + *s++ - '0';
	    }
	    SpecTbls = True;
	    TblsSize = TblsSize * sizeof(heapT);
	    break;
	  default:
	    fprintf(DbgFile, "fcp: illegal option %c\n", *s++);
	    argc = 0;
	    break;
	  }
	}
	break;
      case 's':
	s++;
	DevsTime = 0;
	while(isdigit(*s)) {
	  DevsTime = DevsTime*10 + *s++ - '0';
	}
	SpecDevsTime = True;
	break;
      default:
	fprintf(DbgFile, "fcp: illegal option %c\n", *s++);
	argc = 0;
	break;
      }
    }
  }
  if (argc != 1) {
    fprintf(DbgFile, "Usage: fcp ");
#ifdef	DEBUG
    fprintf(DbgFile, "[-BpcgoauvG] ");
#endif
    fprintf(DbgFile, "-[P[Name]][-[r][l][h][t][tr][tb][s]Num] filename\n");
    return(False);
  }
  if (LinkSize < 0) {
    fprintf(DbgFile, "invalid link size\n");
    return(False);
  }
  if (HeapSize == 0) {
    fprintf(DbgFile, "invalid heap size\n");
    return(False);
  }
  FileName = *argv;
  {
    register int Length = strlen(FileName);

    if (strcmp((FileName+Length-4), ".bin") == 0) {
      BootType = EmulationBoot;
    }
    else {
      if (strcmp((FileName+Length-2), ".o") == 0) {
	BootType = NativeBoot;
      }
    }
  }
  return(True);
}

produce_constants()
{
  if (ended_heap((HP + (ConstSize*7)))) {
    return(False);
  }
  Nil = HP;
  *HP++ = Word(0, NilTag);
  SVRMarker = HP;
  *HP++ = Word(0, NilTag);

  Constants[FailedC] = produce_string("failed");
  Constants[GCC] = produce_string("gc_done");
  Constants[IdleC] = produce_string("idle");
  Constants[BootC] = produce_string("booted");
  Constants[UnknownC] = produce_string("unknown");
  Constants[DoneC] = produce_string("done");
  Constants[FalseC] = produce_string("false");
  Constants[UnboundC] = produce_string("unbound");
  Constants[ErrorC] = produce_string("error");

  Constants[MACHINE] = produce_string("MACHINE");
  Constants[SIGNAL] = produce_string("SIGNAL");
  Constants[SYSTEM] = produce_string("SYSTEM");

  Constants[ErHPOVFL] = produce_string("Heap Overflow");
  Constants[ErTRLSOVFL] = produce_string("Trails Overflow");
  Constants[ErTBLSOVFL] = produce_string("Tabels Overflow");
  Constants[ErHPSPACE] = produce_string("No Heap Space");
  Constants[ErLINKSPACE] = produce_string("No Link Space");
  Constants[ErUDFDCD] = produce_string("Undefined Code");
  Constants[ErINVLDOBJ] = produce_string("Invalid Object");
  Constants[ErBADARG] = produce_string("Bad Argument");
  Constants[ErEXITREQ] = produce_string("Exit Requested");
  Constants[ErNOTOPN] = produce_string("Not Open");
  Constants[ErNODEVSPC] = produce_string("No Device Space");
  Constants[ErNOTFND] = produce_string("Not Found");

  Constants[WeC] = produce_string("we");
  Constants[RoC] = produce_string("ro");
  Constants[BothC] = produce_string("both");
  Constants[BindingC] = produce_string("binding");
  Constants[ValueC] = produce_string("value");
  Constants[CommitC] = produce_string("commit");
  Constants[LockC] = produce_string("lock");
  Constants[Id_QueryC] = produce_string("id_query");
  Constants[IdentifyC] = produce_string("identify");
  Constants[ReferenceC] = produce_string("reference");
  Constants[KnownC] = produce_string("known");
  Constants[SharedC] = produce_string("shared");
  Constants[ActiveC] = produce_string("active");
  Constants[InActiveC] = produce_string("inactive");
  Constants[EmptyC] = produce_string("empty");
  Constants[Non_EmptyC] = produce_string("non_empty");
  Constants[LocalC] = produce_string("local");
  Constants[RemoteC] = produce_string("remote");
  Constants[RequestedC] = produce_string("requested");
  Constants[UnRequestedC] = produce_string("unrequested");
  Constants[ReferencedC] = produce_string("referenced");
  Constants[UnReferencedC] = produce_string("unreferenced");
  Constants[AtomicC] = produce_string("atomic");
  Constants[NonAtomicC] = produce_string("non_atomic");
  Constants[IncrementalC] = produce_string("incremental");
  Constants[ExportedC] = produce_string("exported");
  Constants[GarbageC] = produce_string("garbage");
  Constants[MultipleC] = produce_string("multiple_occurrences");
  Constants[ExcessC] = produce_string("excess_term");
  Constants[DoorsErrorC] = produce_string("doorsError");
  Constants[DoorsEventC] = produce_string("doorsEvent");
  Constants[DoorsRequestC] = produce_string("doorsRequest");
  Constants[DoorsResponseC] = produce_string("doorsResponse");

  Constants[ReadC] = produce_string("read");
  Constants[WriteC] = produce_string("write");
  Constants[ExceptC] = produce_string("except");
  Constants[SignalC] = produce_string("signal");
  return(True);
}

init_stats()
{
  Creations = Creations - Terminations;
  Suspensions = Suspensions - (Activations + Failures + Losses);
  Activations = 0;
  Failures = 0;
  Losses = 0;
  Switches = 0;
  Reductions = 0;
  Terminations = 0;
  CpuTime = 0;
  GCTime = 0;
  Collections = 0;
  FreedAverage = 0;
  CopiedAverage = 0;
  GCMinFlt = 0;
  GCMajFlt = 0;
}
