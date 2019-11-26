/**********************************************************************

  Audacity: A Digital Audio Editor

  EffectsPrefs.cpp

  Brian Gunlogson
  Joshua Haberman
  Dominic Mazzoni
  James Crook


*******************************************************************//**

\class EffectsPrefs
\brief A PrefsPanel for general GUI prefernces.

*//*******************************************************************/

#include "../Audacity.h" // for USE_* macros
#include "EffectsPrefs.h"

#include "../Experimental.h"

#include <wx/choice.h>
#include <wx/defs.h>

#include "../Languages.h"
#include "../PluginManager.h"
#include "../Prefs.h"
#include "../ShuttleGui.h"

EffectsPrefs::EffectsPrefs(wxWindow * parent, wxWindowID winid)
:  PrefsPanel(parent, winid, _("Effects"))
{
   Populate();
}

EffectsPrefs::~EffectsPrefs()
{
}

ComponentInterfaceSymbol EffectsPrefs::GetSymbol()
{
   return EFFECTS_PREFS_PLUGIN_SYMBOL;
}

wxString EffectsPrefs::GetDescription()
{
   return _("Preferences for Effects");
}

wxString EffectsPrefs::HelpPageName()
{
   return "Effects_Preferences";
}

void EffectsPrefs::Populate()
{
   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

namespace {

// Rather than hard-code an exhaustive list of effect families in this file,
// pretend we don't know, but discover them instead by querying the module and
// effect managers.

// But then we would like to have prompts with accelerator characters that are
// distinct.  We collect some prompts in the following map.

// It is not required that each module be found here, nor that each module
// mentioned here be found.
const std::map< wxString, wxString > SuggestedPrompts{

/* i18n-hint: Audio Unit is the name of an Apple audio software protocol */
   { wxT("AudioUnit"), XO("Audio Unit") },

/* i18n-hint: abbreviates "Linux Audio Developer's Simple Plugin API"
   (Application programming interface)
 */
   { wxT("LADSPA"),    XO("&LADSPA") },

/* i18n-hint: abbreviates
   "Linux Audio Developer's Simple Plugin API (LADSPA) version 2" */
   { wxT("LV2"),       XO("LV&2") },

/* i18n-hint: "Nyquist" is an embedded interpreted programming language in
 Audacity, named in honor of the Swedish-American Harry Nyquist (or Nyqvist).
 In the translations of this and other strings, you may transliterate the
 name into another alphabet.  */
   { wxT("Nyquist"),   XO("N&yquist") },

/* i18n-hint: Vamp is the proper name of a software protocol for sound analysis.
   It is not an abbreviation for anything.  See http://vamp-plugins.org */
   { wxT("Vamp"),      XO("&Vamp") },

/* i18n-hint: Abbreviates Virtual Studio Technology, an audio software protocol
   developed by Steinberg GmbH */
   { wxT("VST"),       XO("V&ST") },

};

// Collect needed prompts and settings paths, at most once, on demand
struct Entry {
   wxString prompt; // untranslated
   wxString setting;
};
static const std::vector< Entry > &GetModuleData()
{
   struct ModuleData : public std::vector< Entry > {
      ModuleData() {
         auto &pm = PluginManager::Get();
         for (auto plug = pm.GetFirstPlugin(PluginTypeModule);
              plug;
              plug = pm.GetNextPlugin(PluginTypeModule)) {
            auto internal = plug->GetEffectFamily();
            if ( internal.empty() )
               continue;

            wxString prompt;
            auto iter = SuggestedPrompts.find( internal );
            if ( iter == SuggestedPrompts.end() )
               // For the built-in modules this Msgid includes " Effects",
               // but those strings were never shown to the user,
               // and the prompts in the table above do not include it.
               // If there should be new modules, it is not important for them
               // to follow the " Effects" convention, but instead they can
               // have shorter msgids.
               prompt = plug->GetSymbol().Msgid();
            else
               prompt = iter->second;

            auto setting = pm.GetPluginEnabledSetting( *plug );

            push_back( { prompt, setting } );
         }
         // Guarantee some determinate ordering
         std::sort( begin(), end(),
            []( const Entry &a, const Entry &b ){
               return a.setting < b.setting;
            }
         );
      }
   };
   static ModuleData theData;
   return theData;
}

}

void EffectsPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(_("Enable Effects"));
   {
      for ( const auto &entry : GetModuleData() )
      {
         S.TieCheckBox(
            GetCustomTranslation( entry.prompt ),
            entry.setting,
            true
         );
      }
   }
   S.EndStatic();

   S.StartStatic(_("Effect Options"));
   {
      S.StartMultiColumn(2);
      {
         wxArrayStringEx visualgroups{
            _("Sorted by Effect Name") ,
            _("Sorted by Publisher and Effect Name") ,
            _("Sorted by Type and Effect Name") ,
            _("Grouped by Publisher") ,
            _("Grouped by Type") ,
         };

         wxArrayStringEx prefsgroups{
            wxT("sortby:name") ,
            wxT("sortby:publisher:name") ,
            wxT("sortby:type:name") ,
            wxT("groupby:publisher") ,
            wxT("groupby:type") ,
         };

         wxChoice *c = S.TieChoice(_("S&ort or Group:"),
                                   wxT("/Effects/GroupBy"),
                                   wxT("sortby:name"),
                                   visualgroups,
                                   prefsgroups);
         if( c ) c->SetMinSize(c->GetBestSize());

         S.TieNumericTextBox(_("&Maximum effects per group (0 to disable):"),
                             wxT("/Effects/MaxPerGroup"),
#if defined(__WXGTK__)
                             15,
#else
                             0,
#endif
                             5);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

#ifndef EXPERIMENTAL_EFFECT_MANAGEMENT
   S.StartStatic(_("Plugin Options"));
   {
      S.TieCheckBox(_("Check for updated plugins when Audacity starts"),
                     wxT("/Plugins/CheckForUpdates"),
                     true);
      S.TieCheckBox(_("Rescan plugins next time Audacity is started"),
                     wxT("/Plugins/Rescan"),
                     false);
   }
   S.EndStatic();
#endif

#ifdef EXPERIMENTAL_EQ_SSE_THREADED
   S.StartStatic(_("Instruction Set"));
   {
      S.TieCheckBox(_("&Use SSE/SSE2/.../AVX"),
                    wxT("/SSE/GUI"),
                    true);
   }
   S.EndStatic();
#endif
   S.EndScroller();
}

bool EffectsPrefs::Commit()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   return true;
}

PrefsPanel::Factory
EffectsPrefsFactory = [](wxWindow *parent, wxWindowID winid)
{
   wxASSERT(parent); // to justify safenew
   return safenew EffectsPrefs(parent, winid);
};
