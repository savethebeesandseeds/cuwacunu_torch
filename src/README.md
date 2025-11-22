This is just a prompt i didn't want to miss, 



We need to recap, have a conversation about this project, i feel like we are close but we still have a long way to go. 

So far, we have all the data and the broker communication, the representation and the value estimation networks. We lack an RL setup for the enviroment, and a good vizualization framework. 

You know what we really lack, a good representation framework. ---A champion quorum to drawing. 

I want a representation framework that does not need to be compiled, a backus naur form for the representation and drawing. ---we already have a BNF parser, we would need the language specification an example instruction. 

And make the GUI rounded to be the parser (see G+7 bellow). The navigation commands are a set of two keys or more. So long as the leftside is pressed, one awaits a number then to navigate, for example F+n is to press F and then press a number, same with G, e.g. the long command F+64 ---But F+p means nothing, that way we avoid someone simply being a fast writer. 

You'd have to search the near conversation history for the iinuji files, those are the ncurses basic wrappers. Everything we drawn by a reference artifact to a drawing specification domain language (See G+7 bellow). 

That BNF implementation could assume everything mostly draws like: 
&Arg1 .the application state artifact or pointer (in the global context). 
&Arg2,Arg3,Arg4,.... the drawing artifacts for the current F+n context ---each Arg has .triggers, and a presenter function that yields [starting coordinate, and z level, & scale] for the actual drawing. 


All these can be thread safeguarded so the artifacts can run in concurrency, later.  

The BNF mostly specifies Arg2,Arg3,Arg,..., ---Arg1 is global and should not be expose to the BNF setup, just referenced. Arg2 can be, for example, referencing Arg.1.dataloader to yield the drwaing of a curve. 

Arg1, has these: 

Arg1. intention dictionary (a null void for now, we'll work on the tsodao later) 
Arg1. dataloader
Arg1. representation network
Arg1. value estimation network


Now, i understand is confusing on my side to have to write the things i think, but stay with me for a while, see, ---earlier mention of F+n functions, meant these. 

As the Bakus Naur form has many instructions and one unified BNF language, these are some of the examples for what would specify things like: 
> (F+2) intention dictionary screen 
> (F+3) the dataloader screen
> (F+4) the representation network screen
> (F+5) the value estimation network screen
> (F+6) the reinforment setup screen
> (F+6) the evaluation screen

G+n screen changes are not exposed with BBNF, they are more robust and steady. 
The used can toggle the G+n combo on any screen to see: 
> (G+9) current F's section logs screen
> (G+8) current F's section criptografic guarantees (mockup, well talk about the tsodao later)
> (G+7) current F's BNF 

Help me get this to an excelent finish, im really eager to then continue and share with you the designs of the tsodao. 