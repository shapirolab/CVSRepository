/*

SpiFcp Channel debugging output
William Silverman

Last update by          $Author: bill $
                        $Date: 2005/07/19 14:46:50 $
Currently locked by     $Locker:  $
                        $Revision: 1.6 $
                        $Source: /home/qiana/Repository/Aspic/spi_debug.cp,v $

Copyright (C) 2001, Weizmann Institute of Science - Rehovot, ISRAEL

*/

-language([evaluate,compound,colon]).
-mode(trust).
-export([count_requests/2,
	 format_c/2, format_d/2, format_channel/3,
	 format_channels/4, spi_channels/3,
	 stream_out/4]).

-include(spi_constants).

stream_out(Terms, IdG, Commands, Commands1) + (Done = true, Indent = "") :-

    Terms ? "+",
    string_to_dlist(Indent, LI, [CHAR_SPACE]),
    list_to_string(LI, Indent') |
	self;

    Terms ? "-",
    string_to_dlist(Indent, LI, []),
    LI ? CHAR_SPACE,
    list_to_string(LI', Indent') |
	self;

    Terms ? "-",
    string_to_dlist(Indent, LI, []),
    LI ? CHAR_SPACE,
    LI' =?= [] :
      Indent' = "" |
	self;

    Terms ? Term, Term =\= "+", Term =\= "-",
    string_to_dlist(IdG, LIdG, LI),
    string_to_dlist(Indent, LI, []),
    list_to_string(LIdG, P) :
      Commands ! to_context(computation#display(term, Term,
						[prefix(P),
						known(Done), known(Term),
						close(Done, Done')])) |
	self;

    Terms = [] :
      Done = _,
      IdG = _,
      Indent = _,
      Commands = Commands1.

/*
** Kind == ascii b | c | d
** Status == [..., anchors([BasedAnchor, InstantaneousAnchor)]), ...]
** Out = display_stream
*/
spi_channels(Kind, Status, Out) :-

    Status ? anchors([BasedAnchor, InstantaneousAnchor]) :
      Status' = _ |
	extract_anchored_channels(BasedAnchor, Channels, Instantaneous?),
	extract_anchored_channels(InstantaneousAnchor, Instantaneous, []),
	format_channels(Kind, Channels, Out, []);

    Status ? _Other,
    otherwise |
	self;

    Status =?= [] :
      Kind = _,
      Out = [no_anchors].

  extract_anchored_channels(Anchor, Channels, Channels1) + 
				(Channel = Anchor) :-

    vector(Channel),
    read_vector(SPI_NEXT_CHANNEL, Channel, Channel'),
    Channel =\= Channel',
    Channel' =\= Anchor :
      Channels ! Channel' |
	self;

    vector(Channel),
    read_vector(SPI_NEXT_CHANNEL, Channel, Channel'),
    Channel' =?= Anchor :
      Channels = Channels1.


format_channels(Kind, Channels, Out, Out1) :-

    Kind =?= CHAR_a :
      Channels = _,
      Out = Out1;

    Kind =\= CHAR_a, Kind =\= CHAR_d,
    Channels ? Channel,
    vector(Channel),
    read_vector(SPI_SEND_WEIGHT, Channel, SendWeight),
    read_vector(SPI_RECEIVE_WEIGHT, Channel, ReceiveWeight),
    SendWeight + ReceiveWeight =:= 0 |
	self;

    Channels ? Channel,
    vector(Channel),
    otherwise |
	format_channel,
	format_channel_out(FormattedChannel, Out, Out'),
	self;

    Channels =?= [] :
      Kind = _,
      Out = Out1.


format_c(Channel, FormattedChannel) :-
    format_channel + (Kind = CHAR_c).

format_d(Channel, FormattedChannel) :-
    format_channel + (Kind = CHAR_d).

format_channel(Kind, Channel, FormattedChannel) :-

    read_vector(SPI_CHANNEL_TYPE, Channel, Type),
    read_vector(SPI_CHANNEL_NAME, Channel, Name),
    read_vector(SPI_CHANNEL_REFS, Channel, Refs),
    bitwise_and(Type, SPI_TYPE_MASK, MaskedType),
    MaskedType =?= SPI_BIMOLECULAR,
    read_vector(SPI_SEND_WEIGHT, Channel, SendWeight),
    read_vector(SPI_RECEIVE_WEIGHT, Channel, ReceiveWeight),
    Weight := SendWeight + ReceiveWeight,
    Weight =:= 0,
    Kind =?= CHAR_d :
      FormattedChannel = Name - Refs;

    read_vector(SPI_CHANNEL_TYPE, Channel, Type),
    read_vector(SPI_CHANNEL_NAME, Channel, Name),
    read_vector(SPI_CHANNEL_REFS, Channel, Refs),
    bitwise_and(Type, SPI_TYPE_MASK, MaskedType),
    MaskedType =?= SPI_BIMOLECULAR,
    read_vector(SPI_SEND_WEIGHT, Channel, SendWeight),
    read_vector(SPI_RECEIVE_WEIGHT, Channel, ReceiveWeight),
    Weight := SendWeight + ReceiveWeight,
    Weight > 0 |
	format_channel_b;

    read_vector(SPI_CHANNEL_TYPE, Channel, Type),
    read_vector(SPI_CHANNEL_NAME, Channel, Name),
    read_vector(SPI_CHANNEL_REFS, Channel, Refs),
    bitwise_and(Type, SPI_TYPE_MASK, MaskedType),
    MaskedType =?= SPI_HOMODIMERIZED,
    read_vector(SPI_DIMER_WEIGHT, Channel, DimerWeight),
    DimerWeight =:= 0,
    Kind =?= CHAR_d :
      FormattedChannel = Name - Refs;

    read_vector(SPI_CHANNEL_TYPE, Channel, Type),
    read_vector(SPI_CHANNEL_NAME, Channel, Name),
    read_vector(SPI_CHANNEL_REFS, Channel, Refs),
    bitwise_and(Type, SPI_TYPE_MASK, MaskedType),
    MaskedType =?= SPI_HOMODIMERIZED,
    read_vector(SPI_BLOCKED, Channel, Blocked),
    read_vector(SPI_DIMER_WEIGHT, Channel, DimerWeight),
    DimerWeight > 0,
    read_vector(SPI_DIMER_ANCHOR, Channel, DimerAnchor) |
	count_requests(DimerAnchor, DimerRequests),
	format_channel_h;

    read_vector(SPI_CHANNEL_TYPE, Channel, Type),
    read_vector(SPI_CHANNEL_NAME, Channel, Name),
    read_vector(SPI_CHANNEL_REFS, Channel, Refs),
    bitwise_and(Type, SPI_TYPE_MASK, MaskedType),
    MaskedType =?= SPI_INSTANTANEOUS,
    read_vector(SPI_RECEIVE_ANCHOR, Channel, ReceiveAnchor),
    read_vector(SPI_SEND_ANCHOR, Channel, SendAnchor) |
	count_requests(SendAnchor, SendRequests),
	count_requests(ReceiveAnchor, ReceiveRequests),
	format_channel_i;

    read_vector(SPI_CHANNEL_TYPE, Channel, Type),
    read_vector(SPI_CHANNEL_NAME, Channel, Name),
    read_vector(SPI_CHANNEL_REFS, Channel, Refs),
    bitwise_and(Type, SPI_TYPE_MASK, MaskedType),
    MaskedType =?= SPI_UNKNOWN,
    Kind =?= CHAR_d :
      FormattedChannel = (Name ? Refs);

    otherwise,
    read_vector(SPI_CHANNEL_TYPE, Channel, FullType),
    bitwise_and(FullType, SPI_TYPE_MASK, Type),
    read_vector(SPI_CHANNEL_NAME, Channel, Name),
    Kind =?= CHAR_d,
    read_vector(SPI_CHANNEL_REFS, Channel, Refs) :
      FormattedChannel = Name(Formatted) - Refs |
	format_channel_type;

    otherwise,
    read_vector(SPI_CHANNEL_TYPE, Channel, FullType),
    bitwise_and(FullType, SPI_TYPE_MASK, Type),
    read_vector(SPI_CHANNEL_NAME, Channel, Name),
    Kind =\= CHAR_d :
      FormattedChannel = Name(Formatted) |
	format_channel_type.

  format_channel_type(Type, Formatted) :-

    Type =:= SPI_CHANNEL_ANCHOR :
      Formatted = anchor;

    Type =:= SPI_UNKNOWN :
      Formatted = unknown;

    Type =:= SPI_BIMOLECULAR :
      Formatted = bimolecular;

    Type =:= SPI_HOMODIMERIZED :
      Formatted = homodimerized;

    Type =:= SPI_INSTANTANEOUS :
      Formatted = instantaneous;

    Type =:= SPI_SINK :
      Formatted = sink;

    otherwise :
      Formatted = (type = Type).

  format_channel_b(Channel, Kind, SendWeight, ReceiveWeight, Name, Refs,
				FormattedChannel) :-

    SendWeight =\= 0, ReceiveWeight =\= 0,
    read_vector(SPI_BLOCKED, Channel, Blocked),
    Blocked =?= FALSE,
    CHAR_b =< Kind, Kind =< CHAR_c :
      Refs = _ |
	processor#link(lookup(spicomm, SpiOffset), R1),
	execute(SpiOffset, {SPI_RATE, Channel, W, R2}),
	R2?(R1?(FormattedChannel)) = true(true(Name(W)));

    SendWeight =\= 0, ReceiveWeight =\= 0,
    read_vector(SPI_BLOCKED, Channel, Blocked),
    Blocked =?= TRUE,
    Kind >= CHAR_c :
      Refs = _,
      QM = "?" |
      FormattedChannel = (Name : blocked(SendWeight!, QM(ReceiveWeight)));

    Kind =?= CHAR_c,
    SendWeight =?= 0 :
      Channel = _,
      Refs = _,
      QM = "?",
      FormattedChannel = Name(QM(ReceiveWeight));

    Kind =?= CHAR_c,
    ReceiveWeight =?= 0 :
      Channel = _,
      Refs = _,
      FormattedChannel = Name(SendWeight!);

    Kind =?= CHAR_d,
    read_vector(SPI_BLOCKED, Channel, Blocked),
    read_vector(SPI_SEND_ANCHOR, Channel, SendAnchor),  
    read_vector(SPI_RECEIVE_ANCHOR, Channel, ReceiveAnchor) :
      ReceiveWeight = _,
      SendWeight = _ |
	count_requests(SendAnchor, SendRequests),
	count_requests(ReceiveAnchor, ReceiveRequests),
	format_d_channel_b;

    otherwise :
      Channel = _,
      Kind = _,
      Name = _,
      ReceiveWeight = _,
      Refs = _,
      SendWeight = _,
      FormattedChannel = "".


  format_d_channel_b(Name, Blocked, Refs, SendRequests, ReceiveRequests,
				FormattedChannel) :-

    SendRequests =?= 0 :
      Blocked = _,
      QM = "?",
      FormattedChannel = Name(QM(ReceiveRequests)) - Refs;

    ReceiveRequests =?= 0 :
      Blocked = _,
      FormattedChannel = Name(SendRequests!) - Refs;

    otherwise,
    Blocked = FALSE :
      QM = "?",
      FormattedChannel = Name(SendRequests!, QM(ReceiveRequests)) - Refs;

    otherwise,
    Blocked = TRUE :
      QM = "?",
      FormattedChannel = (Name : blocked(SendRequests!, QM(ReceiveRequests))
				- Refs).

  format_channel_h(Channel, Kind, Blocked, DimerWeight, DimerRequests, Name,
			Refs, FormattedChannel) :-


    CHAR_b =< Kind, Kind =< CHAR_c,
    DimerRequests >= 2,
    Blocked =?= FALSE :
      Channel = _,
      DimerWeight = _,
      Refs = _ |
	processor#link(lookup(spicomm, SpiOffset), R1),
	execute(SpiOffset, {SPI_RATE, Channel, W, R2}),
	R2?(R1?(FormattedChannel)) = true(true(Name(W)));

    Kind =?= CHAR_d,
    DimerRequests >= 2,
    Blocked =?= FALSE :
      Channel = _,
      DimerWeight = _,
      QM = "?",
      FormattedChannel = Name(QM(DimerRequests!)) - Refs;

    Kind =?= CHAR_b,
    otherwise :
      Blocked = _,
      Channel = _,
      DimerRequests = _,
      DimerWeight = _,
      Name = _,
      Refs = _,
      FormattedChannel = "";

    Kind =?= CHAR_c,
    DimerRequests < 2 :
      Blocked = _,
      Channel = _,
      DimerWeight = _,
      Refs = _,
      QM = "?",
      FormattedChannel = Name(QM(DimerRequests!));

    Kind =?= CHAR_d,
    DimerRequests < 2 :
      Blocked = _,
      Channel = _,
      DimerWeight = _,
      QM = "?",
      FormattedChannel = Name(QM(DimerRequests!)) - Refs;

    Kind =?= CHAR_c,
    Blocked = TRUE :
      Channel = _,
      DimerWeight = _,
      Refs = _,
      QM = "?",
      FormattedChannel = (Name : blocked(QM(DimerRequests!)));

    Kind =?= CHAR_d,
    Blocked = TRUE :
      Channel = _,
      DimerWeight = _,
      Refs = _,
      QM = "?",
      FormattedChannel = (Name : blocked(QM(DimerRequests!)) - Refs).


  format_channel_i(Kind, SendRequests, ReceiveRequests, Name, Refs,
				FormattedChannel) :-

    SendRequests =:= 0,
    ReceiveRequests =:= 0,
    Kind =?= CHAR_d :
      FormattedChannel = Name - Refs;

    SendRequests > 0,
    ReceiveRequests =:= 0,
    Kind =?= CHAR_d :
      FormattedChannel = (Name ! SendRequests - Refs);

    SendRequests > 0,
    ReceiveRequests =:= 0,
    Kind =\= CHAR_d :
      FormattedChannel = (Name ! SendRequests - Refs);

    SendRequests > 0,
    ReceiveRequests =:= 0,
    Kind =?= CHAR_c :
      Refs = _,
      FormattedChannel = (Name ! SendRequests);

    SendRequests =:= 0,
    ReceiveRequests > 0,
    Kind =?= CHAR_d :
      QM = "?",
      FormattedChannel = (Name(QM(ReceiveRequests)) - Refs);

    SendRequests =:= 0,
    ReceiveRequests > 0,
    Kind =?= CHAR_c :
      Refs = _,
      QM = "?",
      FormattedChannel = Name(QM(ReceiveRequests));

    SendRequests > 0,
    ReceiveRequests > 0,
    Kind =?= CHAR_c :
      Refs = _,
      QM = "?",
      FormattedChannel = (Name : blocked(SendRequests!, QM(ReceiveRequests)));

    SendRequests > 0,
    ReceiveRequests > 0,
    Kind =?= CHAR_d :
      QM = "?",
      FormattedChannel = (Name : blocked(SendRequests!, QM(ReceiveRequests))
				- Refs);
    otherwise,
    CHAR_b =< Kind, Kind =< CHAR_d :
      Name = _,
      ReceiveRequests = _,
      Refs = _,
      SendRequests = _,
      FormattedChannel = "";

    otherwise :
      Kind = _,
      ReceiveRequests = _,
      Refs = _,
      SendRequests = _,
      FormattedChannel = Name(instantaneous).

  format_channel_out(FormattedChannel, Out, Out1) :-

    FormattedChannel =?= "" :
      Out = Out1;

    FormattedChannel =\= "" :
      Out = [FormattedChannel | Out1].
    

  count_requests(Anchor, Requests) + (Request = Anchor, Counter = 0) :-

    arg(SPI_MESSAGE_LINKS, Request, Links),
    read_vector(SPI_NEXT_MS, Links, Request'),
    Request' =\= Anchor,
    Request' =\= Request,
    Counter++ |
	self;

    otherwise :
      Anchor = _,
      Request = _,
      Requests = Counter.
