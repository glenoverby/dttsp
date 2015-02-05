%% port_doggie.erl
%- Erlang version of port_doggie.py
% 
%- start port_slippers in a separate shell (python port_slippers.py)
%- start erl
%  c(port_doggie).
%- save yourself some typing:
%  PD = port_doggie.
%  PD:start().
%- get some meter data:
%  PD:m().
%- get some spec data:
%  PD:s().
%- ...lather, rinse, repeat...
%- shut down:
%  PD:q().

-module(port_doggie).
-export([start/0,start/3,s/0,s/1,m/0,m/1,q/0]).

-define(CMD_PORT,19001).
-define(SPEC_PORT,19002).
-define(METER_PORT,19003).
-define(DFLT_SPEC_LABEL,4277009102).
-define(DFLT_METER_LABEL,3735928559).
-define(TIMEOUT,1000).

start() ->
    io:format("start()~n",[]),
    start(?CMD_PORT,?SPEC_PORT,?METER_PORT).

start(CmdPort,SpecPort,MeterPort) ->
    CPid = spawn(
	     fun() -> server("localhost",CmdPort,SpecPort,MeterPort) end
	    ),
    io:format("server is ~p~n",[CPid]),
    register(cmdr, CPid).

%%----------------------------------------
%% pass along requests for spectrum, meter
%%----------------------------------------

server(ToHost,ToPort,SpecPort,MeterPort) ->
    {ok, CmdSock} = gen_udp:open(0,[binary]),
    io:format("server socket is ~p~n",[CmdSock]),
    % here's why this is a silly example:
    {ok,SpecSock} = gen_udp:open(SpecPort,[binary,{recbuf,32768}]),
    io:format("spec sucker socket is ~p~n",[SpecSock]),
    {ok,MeterSock} = gen_udp:open(MeterPort,[binary]),
    io:format("meter sucker socket is ~p~n",[MeterSock]),
    loop(CmdSock,ToHost,ToPort,SpecSock,MeterSock).

%%-
%% send a command string, wait for response
%% expects an Erlang string (list), converts to packed binary
%%-
ask(Req,Sock,Host,Port) ->
    io:format("ask sending ~p to ~p,~p~n",[Req,Host,Port]),
    Pkt = list_to_binary(Req),
    gen_udp:send(Sock,Host,Port,[Pkt]),
    receive
    	{udp,_Serv,_Addr,_Port,<<"ok">>} ->
	    io:format("ask gets ok from server~n",[]);
	{udp,_Serv,_Addr,_Port,<<"error">>} ->
	    io:format("ask gets error from server~n",[]);
	Other ->
	    io:format("ask sees Other ~p~n",[Other])
    end.

%%-
%% absorb binary payload of a Type
%%-
suck(Sock,Type) ->
    io:format("suck ~p from ~p~n",[Type,Sock]),
    receive
	{udp,Sock,Host,Port,Pkt} ->
	    io:format("~p ~p ~p ~p~n",[Type,Sock,Host,Port]),
	    {Label,Vals} = brkout(Pkt),
	    io:format("~p ~p ~p~n",[Type,Label,Vals]);
	Other ->
	    io:format("~p garbage ~p~n",[Type,Other])
    after
	?TIMEOUT ->
	    io:format("suck times out~n",[])
    end.

%%-
%% main command loop
%%-
loop(CmdSock,CmdHost,CmdPort,SpecSock,MeterSock) ->
    receive
	{From,spectrum,Label} ->
	    io:format("loop sees spectrum req from ~p, label ~p~n",[From,Label]),
	    Cmd = "reqSpectrum " ++ integer_to_list(Label) ++ "\n",
	    ask(Cmd,CmdSock,CmdHost,CmdPort),
	    suck(SpecSock,"spec"),
	    From ! {self(),spectrum,ok},
	    loop(CmdSock,CmdHost,CmdPort,SpecSock,MeterSock);

	{From,meter,Label} ->
	    io:format("loop sees meter req from ~p, label ~p~n",[From,Label]),
	    Cmd = "reqMeter " ++ integer_to_list(Label) ++ "\n",
	    ask(Cmd,CmdSock,CmdHost,CmdPort),
	    suck(MeterSock,"meter"),
	    From ! {self(),meter,ok},
	    loop(CmdSock,CmdHost,CmdPort,SpecSock,MeterSock);

	{From,stop} ->
	    io:format("loop sees stop req by ~p~n",[From])
    end.

%%---------------------------------
%% "local" user interface functions
%%---------------------------------

%%-
%% initiate request for spectrum
%%-

s() ->
    io:format("s()~n",[]),
    s(?DFLT_SPEC_LABEL).

s(Label) ->
    io:format("s(~p)~n",[Label]),
    cmdr ! {self(),spectrum,Label},
    receive
	{_,spectrum,ok} ->
	    io:format("s() received ack~n",[]);
	_ ->
	    io:format("s() failed to receive ack~n",[])
    end.

%%-
%% initiate request for meter
%%-

m() ->
    io:format("m()~n",[]),
    m(?DFLT_METER_LABEL).

m(Label) ->
    io:format("m(~p)~n",[Label]),
    cmdr ! {self(),meter,Label},
    receive
	{_,meter,ok} ->
	    io:format("m() received ack~n",[]);
	_ ->
	    io:format("m() failed to receive ack~n",[])
    end.

%%-
%% shut down
%%-
q() ->
    io:format("q()~n",[]),
    cmdr ! {self(),stop}.

%%-
%% extract 32-bit label and float vector from binary payload
%%-
brkout(Bin) ->
    <<Label:32,Rest/binary>> = Bin,
    {Label,brkoutf(Rest)}.

%%-
%% brkoutf
%% convert single binary containing 32-bit packed floats to list
%%-

brkoutf(Bin) ->
    brkoutf(Bin,[]).
brkoutf(Bin,Acc) ->
    <<V:32/float,Rest/binary>> = Bin,
    Vals = [V|Acc],
    if
	size(Rest) > 0 ->
	    brkoutf(Rest,Vals);
	true ->
	    lists:reverse(Vals)
    end.
