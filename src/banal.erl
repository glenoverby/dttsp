%% banal.erl

-module(banal).
-export([replicate/2,
	 brkinf/1,
	 brkoutf/1]).

%%[<<Z:32/float>> || Z <- lists:map(fun(_X) -> random:uniform() end, lists:seq(0,99))].
%%lists:map(fun(_I) -> X = -96.0*random:uniform(), <<X:32/float>> end, lists:seq(0,4096)).

%% replicate
%% return list with N copies of Obj

replicate(N,Obj) ->
    replicate(N,Obj,[]).
replicate(0,_Obj,Acc) ->
    Acc;
replicate(N,Obj,Acc) ->
    replicate(N-1,Obj,[Obj|Acc]).

%% brkinf
%% convert list of floats to single binary of 32-bit packed values

brkinf(Flts) ->
    list_to_binary([<<F:32/float>> || F <- Flts]).

%% brkoutf
%% return single binary containing 32-bit packed floats as list

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
