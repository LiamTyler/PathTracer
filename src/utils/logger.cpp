#include "utils/logger.hpp"
#include "configuration.hpp"

Logger g_Logger;

PrintModifier::PrintModifier( Color c, Emphasis e ) :
    color( c ),
    emphasis( e )
{
}

std::ostream& operator<<( std::ostream& out, const PrintModifier& mod )
{
    return out << "\033[" << mod.emphasis << ";" << mod.color << "m";
}

void Logger::Init( const std::string& filename, bool useColors )
{
    AddLocation( "stdout", &std::cout, useColors );
    if ( filename != "" )
    {
        AddLocation( "configFileOutput", filename );
    }
}

void Logger::Shutdown()
{
    outputs.clear();
}
