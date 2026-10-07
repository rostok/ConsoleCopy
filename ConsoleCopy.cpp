#include <plugin.hpp>
#include <algorithm>
#include <string>
#include <vector>
namespace {
const GUID Id{0x5e5a8d56,0x30ac,0x4bc4,{0x8c,0x59,0xa1,0x77,0xd9,0x1c,0x32,0x46}};
const GUID MenuId{0xa2d66912,0xb68c,0x42d7,{0xa7,0x53,0x74,0x64,0x08,0x7a,0x39,0x17}};
PluginStartupInfo api{};
FARSTANDARDFUNCTIONS fsf{};
enum class Mode { None, Stream, Column };
Mode mode=Mode::None;
HANDLE console=INVALID_HANDLE_VALUE;
SMALL_RECT viewport{};
int width=0,height=0;
COORD anchor{},tip{};
std::vector<CHAR_INFO> cells;
unsigned long long clicks=0, candidates=0, starts=0, copies=0;
DWORD lastReadError=0;
int pos(int x,int y){return y*width+x;}
bool hidden(){
  WindowInfo wi{sizeof(wi)};
  wi.Pos=-1; // ACTL_GETWINDOWINFO: -1 means the current Far window.
  PanelInfo a{sizeof(a)},b{sizeof(b)};
  return api.AdvControl && api.PanelControl &&
    api.AdvControl(&Id,ACTL_GETWINDOWINFO,0,&wi) && wi.Type==WTYPE_PANELS &&
    api.PanelControl(PANEL_ACTIVE,FCTL_GETPANELINFO,0,&a) &&
    api.PanelControl(PANEL_PASSIVE,FCTL_GETPANELINFO,0,&b) &&
    !(a.Flags&PFLAGS_VISIBLE) && !(b.Flags&PFLAGS_VISIBLE);
}
COORD clamp(COORD p){
  p.X=static_cast<SHORT>(std::clamp<int>(p.X,0,width-1));
  p.Y=static_cast<SHORT>(std::clamp<int>(p.Y,0,height-1));
  return p;
}
bool marked(int x,int y){
  if(mode==Mode::Column)
    return x>=std::min(anchor.X,tip.X)&&x<=std::max(anchor.X,tip.X)&&
           y>=std::min(anchor.Y,tip.Y)&&y<=std::max(anchor.Y,tip.Y);
  return pos(x,y)>=std::min(pos(anchor.X,anchor.Y),pos(tip.X,tip.Y)) &&
         pos(x,y)<=std::max(pos(anchor.X,anchor.Y),pos(tip.X,tip.Y));
}
void paint(){
  if(console==INVALID_HANDLE_VALUE||cells.empty())return;
  std::vector<CHAR_INFO> row(width);
  for(int y=0;y<height;++y){
    for(int x=0;x<width;++x){
      row[x]=cells[pos(x,y)];
      if(mode!=Mode::None&&marked(x,y)){
        WORD a=row[x].Attributes;
        row[x].Attributes=static_cast<WORD>((a&0xff00)|((a&15)<<4)|((a&240)>>4));
      }
    }
    SMALL_RECT r{viewport.Left,static_cast<SHORT>(viewport.Top+y),
                 viewport.Right,static_cast<SHORT>(viewport.Top+y)};
    WriteConsoleOutputW(console,row.data(),{static_cast<SHORT>(width),1},{0,0},&r);
  }
}
void reset(){
  if(mode!=Mode::None){mode=Mode::None;paint();}
  cells.clear();
  if(console!=INVALID_HANDLE_VALUE)CloseHandle(console);
  console=INVALID_HANDLE_VALUE;
}
bool begin(Mode m,COORD point){
  lastReadError=0;
  console=CreateFileW(L"CONOUT$",GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
  if(console==INVALID_HANDLE_VALUE){lastReadError=GetLastError();return false;}
  CONSOLE_SCREEN_BUFFER_INFO info{};
  if(!GetConsoleScreenBufferInfo(console,&info)){lastReadError=GetLastError();reset();return false;}
  viewport=info.srWindow;
  width=viewport.Right-viewport.Left+1;
  height=viewport.Bottom-viewport.Top+1;
  if(width<=0||height<=0){reset();return false;}
  cells.resize(static_cast<size_t>(width)*height);
  for(int y=0;y<height;++y){
    SMALL_RECT r{viewport.Left,static_cast<SHORT>(viewport.Top+y),
                 viewport.Right,static_cast<SHORT>(viewport.Top+y)};
    if(!ReadConsoleOutputW(console,cells.data()+pos(0,y),
      {static_cast<SHORT>(width),1},{0,0},&r)){lastReadError=GetLastError();reset();return false;}
  }
  mode=m;anchor=tip=clamp(point);++starts;paint();return true;
}
std::wstring selection(){
  std::wstring result;
  int top=std::min(anchor.Y,tip.Y),bottom=std::max(anchor.Y,tip.Y);
  for(int y=top;y<=bottom;++y){
    int left,right;
    if(mode==Mode::Column){
      left=std::min(anchor.X,tip.X);right=std::max(anchor.X,tip.X);
    }else{
      bool forward=pos(anchor.X,anchor.Y)<=pos(tip.X,tip.Y);
      COORD first=forward?anchor:tip,last=forward?tip:anchor;
      left=y==first.Y?first.X:0;right=y==last.Y?last.X:width-1;
    }
    std::wstring line;
    for(int x=left;x<=right;++x){
      wchar_t ch=cells[pos(x,y)].Char.UnicodeChar;
      line+=ch?ch:L' ';
    }
    if(mode==Mode::Stream)while(!line.empty()&&line.back()==L' ')line.pop_back();
    if(y!=top)result+=L"\r\n";
    result+=line;
  }
  return result;
}
}
extern "C" void WINAPI GetGlobalInfoW(GlobalInfo* p){
  p->StructSize=sizeof(*p);p->MinFarVersion=MAKEFARVERSION(3,0,0,1905,VS_RELEASE);
  p->Version=MAKEFARVERSION(1,0,0,3,VS_RELEASE);p->Guid=Id;
  p->Title=L"ConsoleCopy";p->Description=L"Mouse selection of console output";
  p->Author=L"Far Manager contributors";
}
extern "C" void WINAPI SetStartupInfoW(const PluginStartupInfo* p){
  api=*p;
  if(p->FSF){fsf=*p->FSF;api.FSF=&fsf;}
  else api.FSF=nullptr;
}
extern "C" void WINAPI GetPluginInfoW(PluginInfo* p){
  p->StructSize=sizeof(*p);
  p->Flags=PF_PRELOAD;
  static const wchar_t* names[]={L"ConsoleCopy"};
  p->PluginMenu.Guids=&MenuId;
  p->PluginMenu.Strings=names;
  p->PluginMenu.Count=1;
}
extern "C" HANDLE WINAPI OpenW(const OpenInfo*){
  if(api.Message){
    const std::wstring status=L"Klikniecia: "+std::to_wstring(clicks)+L", proby: "+std::to_wstring(candidates)+L", starty: "+std::to_wstring(starts)+L", kopie: "+std::to_wstring(copies);
    const std::wstring error=L"Blad odczytu konsoli: "+std::to_wstring(lastReadError);
    const wchar_t* lines[]={
      L"ConsoleCopy",
      L"Po Ctrl+O: Shift + przeciągnięcie zaznacza tekst.",
      L"Ctrl + przeciągnięcie zaznacza prostokąt.",
      L"Puszczenie przycisku kopiuje do schowka.",
      status.c_str(),
      error.c_str(),
      L"\x01",
      L"OK"
    };
    api.Message(&Id,&MenuId,FMSG_LEFTALIGN,nullptr,lines,
                sizeof(lines)/sizeof(lines[0]),1);
  }
  return nullptr;
}
extern "C" intptr_t WINAPI ProcessConsoleInputW(ProcessConsoleInputInfo* p){
  if(p->Rec.EventType!=MOUSE_EVENT)return 0;
  const auto& click=p->Rec.Event.MouseEvent;
  if(click.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED)++clicks;
  if(!(click.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED) || click.dwEventFlags || !hidden())return 0;
  DWORD mods=click.dwControlKeyState;
  if(mods&(LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))return 0;
  Mode requested=mods&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)?Mode::Column:
                 mods&SHIFT_PRESSED?Mode::Stream:Mode::None;
  if(requested==Mode::None)return 0;
  ++candidates;
  if(!begin(requested,click.dwMousePosition))return 0;

  // Far sends only mouse clicks to ProcessConsoleInputW. Read the remainder
  // of this drag directly, then return unrelated input to Far's queue.
  HANDLE input=CreateFileW(L"CONIN$",GENERIC_READ|GENERIC_WRITE,
    FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
  if(input==INVALID_HANDLE_VALUE){lastReadError=GetLastError();reset();return 1;}
  std::vector<INPUT_RECORD> deferred;
  bool completed=false;
  for(;;){
    INPUT_RECORD record{};
    DWORD count=0;
    if(!ReadConsoleInputW(input,&record,1,&count)||count!=1){lastReadError=GetLastError();break;}
    if(record.EventType==MOUSE_EVENT){
      const auto& e=record.Event.MouseEvent;
      if(e.dwEventFlags==MOUSE_WHEELED||e.dwEventFlags==MOUSE_HWHEELED){
        deferred.push_back(record);
        continue;
      }
      COORD point{static_cast<SHORT>(e.dwMousePosition.X-viewport.Left),
                  static_cast<SHORT>(e.dwMousePosition.Y-viewport.Top)};
      tip=clamp(point);
      if(!(e.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED)){
        completed=true;
        break;
      }
      paint();
    }else if(record.EventType==KEY_EVENT){
      deferred.push_back(record);
      const WORD key=record.Event.KeyEvent.wVirtualKeyCode;
      if(record.Event.KeyEvent.bKeyDown && key!=VK_SHIFT && key!=VK_CONTROL &&
         key!=VK_MENU)break;
    }else{
      deferred.push_back(record);
      if(record.EventType==FOCUS_EVENT||record.EventType==WINDOW_BUFFER_SIZE_EVENT)break;
    }
  }
  if(completed){
    ++copies;
    std::wstring text=selection();
    Mode copied=mode;
    reset();
    if(api.FSF&&api.FSF->CopyToClipboard)
      api.FSF->CopyToClipboard(copied==Mode::Column?FCT_COLUMN:FCT_STREAM,text.c_str());
  }else reset();
  if(!deferred.empty()){
    DWORD written=0;
    WriteConsoleInputW(input,deferred.data(),static_cast<DWORD>(deferred.size()),&written);
  }
  CloseHandle(input);
  return 1;
}
extern "C" void WINAPI ExitFARW(const ExitInfo*){reset();}

