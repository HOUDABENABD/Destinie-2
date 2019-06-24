#include "Reversion.h"
#include "Retraite.h"

/**/#define JB  1

  /**
   * Calcule le montant de reversion pour un individu
   */
  Reversion::Reversion(Indiv & X, Indiv & Y, int t, int legRetroMax) :
      rev(0), rev_rg(0), rev_fp(0), rev_in(0), rev_ag(0), rev_ar(0), rev_ag_ar(0), ageliq_rev(0), nbEnfCharge(0),idConj(Y.Id)
  { 
    nbEnfCharge = X.NbEnfC(t);
    int age_y = 1900 + t - Y.anaiss;  
    int age_x = 1900 + t - X.anaiss;
  
    double agetest_y = arr_mois(t-Y.anaiss%1900, -Y.moisnaiss); //TODO
    Leg l_y = Leg(Y, age_y, min(Y.anaiss+age_y,legRetroMax));
    Retraite retraite_y(Y,t); 
    // copie
	retraite_y.primoliq = Y.retr->primoliq;
	retraite_y.totliq = Y.retr->totliq;
	retraite_y.liq = Y.retr->liq;
	retraite_y.revaloDir(t);
	// Si le conjoint décédé n'avait pas tout liquidé
	// on calcule une pension théorique
  
	ptr<DroitsRetr> dr = make_shared<DroitsRetr>(Y,l_y,agetest_y);
	if( !retraite_y.totliq) {
     
     dr->l.AgeMinRG = 0;
     dr->l.AgeMinFP = 0;
     
     dr->durees_base();
     dr->durees_majo();
     dr->DecoteSurcote();
     
     if( !retraite_y.primoliq ) {
        dr->tauxliq_rg=1;
        dr->tauxliq_ar=1;
        dr->tauxliq_fp=1;
        dr->Liq();
     }
     else if( !retraite_y.totliq ){
        dr->tauxliq_rg=1;
        dr->tauxliq_ar=1;
        dr->SecondLiq();
     }
     retraite_y.liq = dr;
     retraite_y.totliq = dr;
     retraite_y.primoliq = dr;    
  }
  
  
  retraite_y.revaloDir(t);
  
  double duree_rg_maj = retraite_y.liq->duree_rg_maj;
  double duree_in_maj = retraite_y.liq->duree_in_maj;

#if defined(JB)
  /* @JB20190623 Objet temporaire Retraite pour X aussi */
      double agetest_x = arr_mois(t-X.anaiss%1900, -X.moisnaiss); //TODO
      Leg lx = Leg(X, age_x, min(X.anaiss+age_x,legRetroMax));

      Retraite retraite_x(X,t);
      // copie
      retraite_x.primoliq = X.retr->primoliq;
      retraite_x.totliq = X.retr->totliq;
      retraite_x.liq = X.retr->liq;
      retraite_x.revaloDir(t);
      // Si le conjoint survivant bénéficiaire n'a pas tout liquidé
      // on calcule une pension théorique
      
      ptr<DroitsRetr> drx = make_shared<DroitsRetr>(X,lx,agetest_x);
      if( !retraite_x.totliq) {
          
          drx->l.AgeMinRG = 0;
          drx->l.AgeMinFP = 0;
          
          drx->durees_base();
          drx->durees_majo();
          drx->DecoteSurcote();
          
          if( !retraite_x.primoliq ) {
              drx->tauxliq_rg=1;
              drx->tauxliq_ar=1;
              drx->tauxliq_fp=1;
              drx->Liq();
          }
          else if( !retraite_x.totliq ){
              drx->tauxliq_rg=1;
              drx->tauxliq_ar=1;
              drx->SecondLiq();
          }
          retraite_x.liq = drx;
          retraite_x.totliq = drx;
          retraite_x.primoliq = drx;
      }
      
      
      retraite_x.revaloDir(t);
      
      double duree_rg_maj_x = retraite_x.liq->duree_rg_maj;
      double duree_in_maj_x = retraite_x.liq->duree_in_maj;

      
  /* @JB20190623 ... Objet temporaire pour X également *****************************************/
#endif
      
  // Puis on simule la réversion
  // Reversions sans conditions de ressources
  // Attention pour les fonctionnaires, on ne vérifie pas que les enfants sont communs à la personne décédée et au bénéficiaire
  
  rev_fp = M->TauxRevFP[t]    * retraite_y.pension_fp;

#if !defined(JB)
// @JB20190623 Formules refaites plus bas pour prendre en compte X également
  rev_ar = M->TauxRevARRCO[t] * retraite_y.pension_ar;
  rev_ag = M->TauxRevAGIRC[t] * retraite_y.pension_ag;
  rev_ag_ar = M->TauxRevARRCO[t] * retraite_y.pension_ag_ar;
#else
      /* @JB20190623 ****/
     // rev_ar = (double(2)/double(3)) * retraite_y.pension_ar - (double(1)/double(3)) * retraite_x.pension_ar;
	      
	      rev_ar = retraite_x.pension_ar;

      if(rev_ar < 0) rev_ar = 0;

      rev_ag = (double(2)/double(3)) * retraite_y.pension_ag - (double(1)/double(3)) * retraite_x.pension_ag;

      if(rev_ag < 0) rev_ag = 0;

      rev_ag_ar = (double(2)/double(3)) * retraite_y.pension_ag_ar - (double(1)/double(3)) * retraite_x.pension_ag_ar;

      if(rev_ag_ar < 0) rev_ag_ar = 0;
      /******************/
#endif
      
  //rev_fpe= M->TauxRevFP[t]   * Y.pension_fp * (Y.typefp == "FPE");
  //rev_fpt= M->TauxRevFP[t]   * Y.pension_fp * (Y.typefp == "FPT");
  
  rev_in= M->TauxRevInd[t]  * (retraite_y.pension_in-retraite_y.liq->majo_3enf_in);
  // reversion du régime général
  double partmin;
  double rev_lura;
  //double ratio_rg=0;
  Leg l_x = Leg(X, age_x, min(X.anaiss+age_x,legRetroMax));
  
  if (t<104 || l_x.an_leg<2004)
  {
     partmin  = min_max(duree_rg_maj / max(15.0, duree_rg_maj), 0, 1);
     rev_rg = M->TauxRevRG[t] *(retraite_y.pension_rg-retraite_y.liq->majo_3enf_rg);
     rev_rg = min_max(rev_rg, (M->MinRevRG[t] * partmin), M->MaxRevRG[t]); // partmin s'applique uniquement à MinRevRG
  }
  else if (options->SAMRgInUnique || (dr->l.LegSAM >= 2013 && (t >= 117  && retraite_y.liq->t>=117) && !options->SAMSepare)){// la reversion est liquidée apres 2017 et les droits directs associés aussi, on est donc sous LURA
	 partmin = min_max((duree_rg_maj+duree_in_maj) / max(15.0, duree_rg_maj + duree_in_maj), 0, 1);
	 //if (duree_rg_maj+duree_in_maj>0) ratio_rg = duree_rg_maj / (duree_rg_maj+duree_in_maj);
	 double ratio_rg = (duree_rg_maj+duree_in_maj) >0 ? duree_rg_maj / (duree_rg_maj+duree_in_maj) : 0;
	 double ratio_in = (duree_rg_maj+duree_in_maj) >0 ? duree_in_maj / (duree_rg_maj+duree_in_maj) : 0;
     rev_lura =M->TauxRevRG[t] * (retraite_y.pension_rg-retraite_y.liq->majo_3enf_rg+retraite_y.pension_in-retraite_y.liq->majo_3enf_in);
     rev_lura =min_max(rev_lura, (M->MinRevRG[t] * partmin), M->MaxRevRG[t]);
	 rev_rg=rev_lura*ratio_rg;
	 rev_in=rev_lura*ratio_in;
  }
  else
  {
     partmin = min_max(duree_rg_maj / max(15.0, duree_rg_maj + duree_in_maj), 0, 1);
     rev_rg =M->TauxRevRG[t] * (retraite_y.pension_rg-retraite_y.liq->majo_3enf_rg);
     rev_rg =min_max(rev_rg, (M->MinRevRG[t] * partmin), M->MaxRevRG[t]);
  }

  // la reversion est majorée en fonction du nombre d'enfants du bénéficiaire pas de l'assuré...
  double tauxmajo_rg=0;
  double tauxmajo_in=0;
  if(X.nb_enf(X.age(t)) >= 3) {
      tauxmajo_rg = 0.1;
      tauxmajo_in = 0.1;
    }
  rev_rg*=(1+tauxmajo_rg);
  rev_in*=(1+tauxmajo_in);
  
  // Calcul de la reversion globale
  rev = rev_rg + rev_ar + rev_ag + rev_fp + rev_in;
  
  ageliq_rev = age_x;
  

}


// Destinie 2
// Copyright © 2005-2018, Institut national de la statistique et des études économiques
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
