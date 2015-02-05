%% udpechoclient.erl
%% simple Erlang foil to client part of udpecho.py

-module(udpechoclient).

-export([client/1,send_message/1]).

client(Data) ->
    {ok,Socket} = gen_udp:open(0, [binary]),
    io:format("client opened socket ~p~n", [Socket]),
    io:format("client sending ~p~n", [Data]),
    ok = gen_udp:send(Socket,"localhost",50007,Data),
    Result = receive
		 {udp,Socket,_,_,_Bin} = Rsp ->
		     io:format("client got response ~p~n",[Rsp]),
		     ok
	     after 1000 ->
		     io:format("client failed to get timely response~n"),
		     {error, timeout}
	     end,
    gen_udp:close(Socket),
    io:format("client has closed socket ~p~n", [Socket]),
    Result.

send_message(Str) ->
    Data = list_to_binary(Str ++ "\n"),
    Resp = case client(Data) of
	       ok ->
		   io:format("Taa-Daa!~n",[]),
		   ok;
	       {error,timeout} ->
		   io:format("Did you start the server?~n",[]),
		   error
	   end,
    Resp.
