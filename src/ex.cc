/* 
 *  EmuSC - Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "ex.h"

#include <ostream>


Ex::Ex(int _errorNr, std::string _errorMsg)
  : _depth(0), _ex(NULL)
{
  errorNr = _errorNr;
  errorMsg = _errorMsg;

}

Ex::Ex(int _errorNr, std::string _errorMsg, const Ex& ex)
  : _depth(ex._depth + 1), _ex(NULL)
{
  errorNr = _errorNr;
  errorMsg = _errorMsg;
  
  _ex = new Ex(ex);
}

Ex::Ex(const Ex& ex)
  : _depth(ex._depth), _ex(NULL), errorNr(ex.errorNr), errorMsg(ex.errorMsg)
{
  if (ex._ex)
    _ex = new Ex(*(ex._ex));
}


Ex::~Ex()
{
  if (_ex)
    delete _ex;
}


std::ostream& operator<< (std::ostream& s, const Ex& ex)
{
  if (ex._ex)
    s << *(ex._ex);

  s << ex._depth << ":  ErrorCode: " << ex.errorNr 
    << ", message: " << ex.errorMsg << std::endl;

  return s;
}
