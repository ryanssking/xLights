#include "PlayListItemFSEQ.h"
#include "wx/xml/xml.h"
#include <wx/notebook.h>
#include "PlayListItemFSEQPanel.h"
#include "../../xLights/AudioManager.h"

PlayListItemFSEQ::PlayListItemFSEQ(wxXmlNode* node) : PlayListItem(node)
{
    _applyMethod = APPLYMETHOD::METHOD_OVERWRITE;
    _fseqFileName = "";
    _overrideAudio = false;
    _audioFile = "";
    _durationMS = 0;
    _fseqFile = nullptr;
    _audioManager = nullptr;
    PlayListItemFSEQ::Load(node);
}

void PlayListItemFSEQ::Load(wxXmlNode* node)
{
    PlayListItem::Load(node);
    _fseqFileName = node->GetAttribute("FSEQFile", "");
    _audioFile = node->GetAttribute("AudioFile", "");
    _overrideAudio = (_audioFile != "");
    _applyMethod = (APPLYMETHOD)wxAtoi(node->GetAttribute("ApplyMethod", ""));
    LoadFiles();
    if (_audioManager != nullptr)
    {
        _durationMS = _audioManager->LengthMS();
    }
    else if (_fseqFile != nullptr)
    {
        _durationMS = _fseqFile->GetLengthMS();
    }
    CloseFiles();
}

void PlayListItemFSEQ::LoadFiles()
{
    CloseFiles();

    if (wxFile::Exists(_fseqFileName))
    {
        _fseqFile = new FSEQFile();
        _fseqFile->Load(_fseqFileName);
    }
    if (_overrideAudio)
    {
        if (wxFile::Exists(_audioFile))
        {
            _audioManager = new AudioManager(_audioFile);
        }
    }
    else
    {
        if (_fseqFile != nullptr)
        {
            if (wxFile::Exists(_fseqFile->GetAudioFileName()))
            {
                _audioManager = new AudioManager(_fseqFile->GetAudioFileName());
            }
        }
    }
}

PlayListItemFSEQ::PlayListItemFSEQ() : PlayListItem()
{
    _applyMethod = APPLYMETHOD::METHOD_OVERWRITE;
    _fseqFileName = "";
    _overrideAudio = false;
    _audioFile = "";
    _durationMS = 0;
    _audioManager = nullptr;
    _fseqFile = nullptr;
}

PlayListItem* PlayListItemFSEQ::Copy() const
{
    PlayListItemFSEQ* res = new PlayListItemFSEQ();
    res->_fseqFileName = _fseqFileName;
    res->_applyMethod = _applyMethod;
    res->_overrideAudio = _overrideAudio;
    res->_audioFile = _audioFile;
    res->_durationMS = _durationMS;
    PlayListItem::Copy(res);

    return res;
}

wxXmlNode* PlayListItemFSEQ::Save()
{
    wxXmlNode * node = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "PLIFSEQ");

    node->AddAttribute("FSEQFile", _fseqFileName);
    node->AddAttribute("ApplyMethod", wxString::Format(wxT("%i"), (int)_applyMethod));

    if (_overrideAudio)
    {
        node->AddAttribute("AudioFile", _audioFile);
    }

    PlayListItem::Save(node);

    return node;
}

void PlayListItemFSEQ::Configure(wxNotebook* notebook)
{
    notebook->AddPage(new PlayListItemFSEQPanel(notebook, this), "FSEQ", true);
}

std::string PlayListItemFSEQ::GetName() const
{
    wxFileName fn(_fseqFileName);
    if (fn.GetName() == "")
    {
        return "FSEQ";
    }
    else
    {
        std::string duration = "";
        if (_durationMS != 0)
        {
            duration = "[" + wxString::Format(wxT("%.3f"), (float)_durationMS / 1000.0).ToStdString() + "]";
        }
        return fn.GetName().ToStdString() + duration;
    }
}

void PlayListItemFSEQ::SetFSEQFileName(const std::string& fseqFileName)
{
    _fseqFileName = fseqFileName;
    LoadFiles();
    if (_audioManager != nullptr)
    {
        _durationMS = _audioManager->LengthMS();
    }
    else if (_fseqFile != nullptr)
    {
        _durationMS = _fseqFile->GetLengthMS();
    }
    CloseFiles();
    _dirty = true;
}

size_t PlayListItemFSEQ::GetPositionMS() const
{
    if (ControlsTiming())
    {
        return _audioManager->Tell();
    }
    
    return 0;
}

void PlayListItemFSEQ::Frame(wxByte* buffer, size_t size, size_t ms, size_t framems)
{
    _fseqFile->ReadData(buffer, size, ms / framems, _applyMethod);
}

void PlayListItemFSEQ::Start()
{
    // load the FSEQ
    // load the audio
    LoadFiles();

    if (ControlsTiming())
    {
        _audioManager->Play(0, _audioManager->LengthMS());
    }
}

void PlayListItemFSEQ::Stop()
{
    _audioManager->Stop();
    CloseFiles();
}

void PlayListItemFSEQ::CloseFiles()
{
    if (_fseqFile != nullptr)
    {
        _fseqFile->Close();
        delete _fseqFile;
        _fseqFile = nullptr;
    }

    if (_audioManager != nullptr)
    {
        delete _audioManager;
        _audioManager = nullptr;
    }
}