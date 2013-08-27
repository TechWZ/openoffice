/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/
#include "gConXhp.hxx"



/*****************************************************************************
 *********************   G C O N X H P W R A P . C X X   *********************
 *****************************************************************************
 * This includes the c code generated by flex
 *****************************************************************************/



/************   I N T E R F A C E   I M P L E M E N T A T I O N   ************/
convert_xhp::convert_xhp(l10nMem& crMemory)
                        : convert_gen_impl(crMemory),
                          meExpectValue(VALUE_NOT_USED),
                          miCntLanguages(0),
                          mcOutputFiles(NULL),
                          msLangText(NULL)
{
  // xhp files are written through a local routine
  mbLoadMode = true;
}



/************   I N T E R F A C E   I M P L E M E N T A T I O N   ************/
convert_xhp::~convert_xhp()
{
  if (mcOutputFiles)
  {
    for (int i = 0; i < miCntLanguages; ++i)
      mcOutputFiles[i].close();
    delete[] mcOutputFiles;
  }
  if (msLangText)
    delete[] msLangText;
}



/**********************   I M P L E M E N T A T I O N   **********************/
namespace XhpWrap
{
#define IMPLptr convert_gen_impl::mcImpl
#define LOCptr ((convert_xhp *)convert_gen_impl::mcImpl)
#include "gConXhp_yy.c"
}


/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::execute()
{
  std::string sLang;
  std::string sFile, sFile2;

  // prepare list with languages
  miCntLanguages = mcMemory.prepareMerge();
  if (mbMergeMode)
  {
    mcOutputFiles  = new std::ofstream[miCntLanguages];
    msLangText     = new std::string[miCntLanguages];

    for (int i = 0; mcMemory.getMergeLang(sLang, sFile); ++i)
    {
      sFile2 = sLang + "/" + msSourceFile;
      sFile  = msTargetPath + sFile2;
      mcOutputFiles[i].open(sFile.c_str(), std::ios::binary); 
      if (!mcOutputFiles[i].is_open())
      {
        if (!convert_gen::createDir(msTargetPath, sFile2))
          throw l10nMem::showError("Cannot create missing directories (" + sFile + ") for writing");

        mcOutputFiles[i].open(sFile.c_str(), std::ios::binary); 
        if (!mcOutputFiles[i].is_open())
          throw l10nMem::showError("Cannot open file (" + sFile + ") for writing");
      }
      msLangText[i] = "xml-lang=\"" + sLang + "\"";
    }
  }

  // run analyzer
  XhpWrap::yylex();

  // dump last line
  copySourceSpecial(NULL,3);
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::setString(char *yytext)
{
  copySourceSpecial(yytext, 0);
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::openTag(char *yytext)
{
  if (meExpectValue == VALUE_IS_VALUE)
  {
    meExpectValue  = VALUE_IS_VALUE_TAG;
//FIX    msCollector   += "\\";
  }
  copySourceSpecial(yytext, 0);
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::closeTag(char *yytext)
{
  STATE newState = meExpectValue;

  switch (meExpectValue)
  {
    case VALUE_IS_VALUE_TAG:
         newState = VALUE_IS_VALUE;
//FIX         msCollector   += "\\";
         break;

    case VALUE_IS_TAG_TRANS:
         if (msKey.size())
           newState = VALUE_IS_VALUE;
         break;

    case VALUE_IS_TAG:
         msKey.clear();
         newState = VALUE_NOT_USED;
         break;
    case VALUE_NOT_USED:
    case VALUE_IS_VALUE:
         break;
  }
  copySourceSpecial(yytext, 0);
  meExpectValue = newState;
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::setId(char *yytext)
{
  int          nL, nE;
  std::string& sText = copySourceSpecial(yytext, 0);


  nL = sText.find("\"");
  nE = sText.find("\"", nL+1);
  if (nL == (int)std::string::npos || nE == (int)std::string::npos)
    return;

  switch (meExpectValue)
  {
    case VALUE_IS_TAG:
    case VALUE_IS_TAG_TRANS:
         msKey = sText.substr(nL+1, nE - nL -1) + msKey;
         break;

    case VALUE_IS_VALUE_TAG:
    case VALUE_NOT_USED:
    case VALUE_IS_VALUE:
         break;
  }
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::setLang(char *yytext)
{
  int          nL, nE;
  std::string  sLang;
  std::string& sText = copySourceSpecial(yytext, 1);


  nL = sText.find("\"");
  nE = sText.find("\"", nL+1);
  if (nL == (int)std::string::npos || nE == (int)std::string::npos)
    return;

  switch (meExpectValue)
  {
    case VALUE_IS_TAG:
         sLang = sText.substr(nL+1, nE - nL -1);
         if (sLang == "en-US")
           meExpectValue = VALUE_IS_TAG_TRANS;
         else
          mcMemory.showError(sLang + " is no en-US language");
         break;

    case VALUE_IS_VALUE_TAG:
         msCollector.erase(msCollector.size() - sText.size() -1);
         break;

    case VALUE_NOT_USED:
    case VALUE_IS_TAG_TRANS:
    case VALUE_IS_VALUE:
         break;
  }
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::setRef(char *yytext)
{
  int          nL, nE;
  std::string& sText = copySourceSpecial(yytext, 0);


  nL = sText.find("\"");
  nE = sText.find("\"", nL+1);
  if (nL == (int)std::string::npos || nE == (int)std::string::npos)
    return;

  switch (meExpectValue)
  {
    case VALUE_IS_TAG:
    case VALUE_IS_TAG_TRANS:
         msKey += "." + sText.substr(nL+1, nE - nL -1);
         break;

    case VALUE_IS_VALUE_TAG:
    case VALUE_NOT_USED:
    case VALUE_IS_VALUE:
         break;
  }
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::openTransTag(char *yytext)
{
  copySourceSpecial(yytext, 0);
  msKey.clear();
  meExpectValue = VALUE_IS_TAG;
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::closeTransTag(char *yytext)
{
  int iType = 0;


  if (meExpectValue == VALUE_IS_VALUE || meExpectValue == VALUE_IS_VALUE_TAG)
  {
    if (msCollector.size() && msCollector != "-")
      mcMemory.setSourceKey(miLineNo, msSourceFile, msKey, msCollector);
    msKey.clear();
    iType = 2;
  }
  meExpectValue = VALUE_NOT_USED;
  copySourceSpecial(yytext, iType);
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::stopTransTag(char *yytext)
{
  copySourceSpecial(yytext, 0);
  meExpectValue = VALUE_NOT_USED;
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::startComment(char *yytext)
{
  mePushValue   = meExpectValue;
  msPushCollect = msCollector;
  meExpectValue = VALUE_NOT_USED;
  copySourceSpecial(yytext, 0);
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::stopComment(char *yytext)
{
  copySourceSpecial(yytext, 0);
  meExpectValue = mePushValue;
  msCollector   = msPushCollect;
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::handleSpecial(char *yytext)
{
  int          nX    = msCollector.size();
  std::string& sText = copySourceSpecial(yytext, 0);


  if (meExpectValue != VALUE_IS_VALUE || meExpectValue != VALUE_IS_VALUE_TAG)
  {
    msCollector.erase(nX);
    if      (sText == "&amp;")
      msCollector += "&";
    else if (sText == "&lt;")
      msCollector += "<";
    else if (sText == "&gt;")
      msCollector += ">";
    else if (sText == "&quot;")
      msCollector += "\"";
  }
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::handleDataEnd(char *yytext)
{
  int nX = msCollector.size();
  copySourceSpecial(yytext, 0);

  if (meExpectValue == VALUE_IS_VALUE || meExpectValue == VALUE_IS_VALUE_TAG)
    msCollector.erase(nX);
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::duplicate(char *yytext)
{
  copySourceSpecial(yytext, 0);

  if (meExpectValue == VALUE_IS_VALUE || meExpectValue == VALUE_IS_VALUE_TAG)
    msCollector += msCollector[msCollector.size()-1];
}



/**********************   I M P L E M E N T A T I O N   **********************/
std::string& convert_xhp::copySourceSpecial(char *yytext, int iType)
{
  bool         doingValue = (meExpectValue == VALUE_IS_VALUE || meExpectValue == VALUE_IS_VALUE_TAG);
  std::string& sText      = copySource(yytext, !doingValue);
  std::string  sLang;
  int          i;


  // Do NOT write data while collecting a value (will be replaced by language text)
  if (doingValue)
    return sText;

  // Handling depends o
  switch (iType)
  {
    case 0: // Used for tokens that are to be copied 1-1, 
            if (mbMergeMode)
            {
              msLine += yytext;
              if (*yytext == '\n')
              {
                for (i = 0; i < miCntLanguages; ++i)
                  writeSourceFile(msLine, i);
                msLine.clear();
              }
            }
            break;

    case 1: // Used for language token, are to replaced with languages
            if (mbMergeMode)
            {
              for (i = 0; i < miCntLanguages; ++i)
              {
                writeSourceFile(msLine, i);
                writeSourceFile(msLangText[i], i);
              }
              msLine.clear();
            }
            break;

    case 2: // Used for token at end of value, language text are to be inserted and then token written
            if (mbMergeMode)
            {
              mcMemory.prepareMerge();
              for (i = 0; i < miCntLanguages; ++i)
              {
                writeSourceFile(msLine, i);
                mcMemory.getMergeLang(sLang, sText);
                writeSourceFile(sText,i);
                std::string sYY(yytext);
                writeSourceFile(sYY, i);
              }
              msLine.clear();
            }
            break;

    case 3: // Used for EOF 
            if (mbMergeMode)
            {
              for (i = 0; i < miCntLanguages; ++i)
                writeSourceFile(msLine, i);
            }
            break;
  }
  return sText;
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_xhp::writeSourceFile(std::string& sText, int inx)
{
  if (sText.size() && mcOutputFiles[inx].is_open())
    mcOutputFiles[inx].write(sText.c_str(), sText.size());
}
