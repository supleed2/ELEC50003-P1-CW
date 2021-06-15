%lex
%%

\s                                                               return 'whitespace'
("-"[0-9]+"mm"|[0-9]+"mm")                                       return 'distance'
([0-9]|[1-8][0-9]|9[0-9]|[12][0-9]{2}|3[0-4][0-9]|35[0-9])"deg"  return 'heading_angle'
([0-9]|[1-8][0-9]|9[0-9]|100)"%"                                 return 'percentage'
[0-9]+"s"                                                        return 'stop_duration'
move                                                             return 'move'
pstop                                                            return 'pstop'
stop                                                             return 'stop'
help                                                             return 'help'
charge\sto                                                       return 'charge_to'
telemetry\sreset                                                 return 'telemetry_reset'
colour                                                           return 'colour'
("red"|"blue"|"green"|"pink"|"orange")                           return 'colour_name'
<<EOF>>                                                          return 'EOF'
.                                                                return 'invalid_command'

/lex

%start command

%%

command
    : expr EOF
    ;

expr
    : move whitespace distance whitespace heading_angle whitespace percentage
        {   
            var inDist = Number(String($3).substr(0, ((String($3).length) - 2)));
            var inHdg = Number(String($5).substr(0, ((String($5).length) - 3)));
            var inSpd = Number(String($7).substr(0, ((String($7).length) - 1)));
            moveCmd(inDist,inHdg,inSpd);
        }
    | stop
        {
            stpCmd();
        }
    | pstop whitespace stop_duration
        {
            var inStpDur = Number(String($3).substr(0, ((String($3).length) - 1)));
            pstpCmd(inStpDur);
        }
    | charge_to whitespace percentage
        {
            var inChrg = Number(String($3).substr(0, ((String($3).length) - 1)));
            chrgCmd(inChrg);
        }
    | colour whitespace colour_name
        {
            var inColour = String($3);
            colourCmd(inColour);
        }
    | telemetry_reset
        {   
            telRst();
        }
    | help
        {
            printHelpDetails();
        }
    ;

