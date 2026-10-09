#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// Vendor APP_Show A/B/C messages and official 2024 ROS 24-byte feedback.
// Feed incrementally: notification boundaries are not protocol boundaries.
class C30dFeedback {
public:
 enum Kind {None,AppWheels,AppParameters,RosState};
 struct Frame {Kind kind=None;int left=0,right=0,speed=0,vx=0,vy=0,vz=0,battery=0;bool zero=false;};
 void reset(){textSize=rosSize=0;}
 bool feed(uint8_t c,Frame& f){
  f=Frame{};bool valid=false;
  if(c=='{'){textSize=0;text[textSize++]=c;}
  else if(textSize){
   if(c<32||c>126||textSize>=sizeof(text)-1)textSize=0;
   else{text[textSize++]=char(c);if(c=='}'){text[textSize]=0;valid=parseText(f);textSize=0;}}
  }
  if(rosSize||c==0x7b)ros[rosSize++]=c;
  if(rosSize==24){
   uint8_t crc=0;for(int i=0;i<22;i++)crc^=ros[i];
   if(ros[0]==0x7b&&ros[23]==0x7d&&crc==ros[22]){
    f.kind=RosState;f.vx=signed16(ros+2);f.vy=signed16(ros+4);f.vz=signed16(ros+6);f.battery=uint16_t(ros[20])<<8|ros[21];f.zero=f.vx==0&&f.vy==0&&f.vz==0;valid=true;rosSize=0;
   }else{memmove(ros,ros+1,23);rosSize=23;while(rosSize&&ros[0]!=0x7b){memmove(ros,ros+1,--rosSize);}}
  }
  return valid;
 }
private:
 char text[96]={};unsigned textSize=0;uint8_t ros[24]={};unsigned rosSize=0;
 static int signed16(const uint8_t* p){return int16_t(uint16_t(p[0])<<8|p[1]);}
 bool parseText(Frame& f){
  int a=0,b=0,v=0,x=0,y=0,end=0;
  bool wheels=false;
  if(sscanf(text,"{A%d:%d:%d:%d:%d}%n",&a,&b,&v,&x,&y,&end)==5&&end&&text[end]==0)wheels=true;
  else{end=0;if(sscanf(text,"{A%d:%d:%d:%d}%n",&a,&b,&v,&x,&end)==4&&end&&text[end]==0)wheels=true;}
  if(!wheels){end=0;if(sscanf(text,"{B%d:%d:%d}%n",&v,&a,&b,&end)==3&&end&&text[end]==0)wheels=true;}
  if(wheels&&a>=0&&a<=255&&b>=0&&b<=255){f.kind=AppWheels;f.left=a;f.right=b;f.zero=a==0&&b==0;return true;}
  if(text[1]=='C'&&sscanf(text,"{C%d:",&a)==1&&a>=0&&a<=3500){f.kind=AppParameters;f.speed=a;return true;}
  return false;
 }
};
