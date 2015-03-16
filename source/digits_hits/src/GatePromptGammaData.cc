/*----------------------
  GATE version name: gate_v7

  Copyright (C): OpenGATE Collaboration

  This software is distributed under the terms
  of the GNU Lesser General  Public Licence (LGPL)
  See GATE/LICENSE.txt for further details
  ----------------------*/

#include "GatePromptGammaData.hh"
#include "GateActorManager.hh"
#include "G4UnitsTable.hh"
#include "G4Material.hh"
#include "G4MaterialTable.hh"
#include "G4SystemOfUnits.hh"

//-----------------------------------------------------------------------------
GatePromptGammaData::GatePromptGammaData()
{
  SetProtonEMin(0);
  SetProtonEMax(150*MeV);
  SetGammaEMin(0);
  SetGammaEMax(10*MeV);
  SetProtonNbBins(250);
  SetGammaNbBins(250);
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
GatePromptGammaData::~GatePromptGammaData()
{
  delete GammaZ;
  delete Ngamma;

  delete pHEpEpg;
  delete pHEpEpgNormalized;
  delete pHEpInelastic;
  delete pHEp;
  delete pHEpInelasticProducedGamma;
  delete pHEpSigmaInelastic;
  delete pTfile;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GatePromptGammaData::SetProtonEMin(double x) { min_proton_energy = x; }
void GatePromptGammaData::SetProtonEMax(double x) { max_proton_energy = x; }
void GatePromptGammaData::SetGammaEMin(double x)  { min_gamma_energy = x; }
void GatePromptGammaData::SetGammaEMax(double x)  { max_gamma_energy = x; }
void GatePromptGammaData::SetProtonNbBins(int x)  { proton_bin = x; }
void GatePromptGammaData::SetGammaNbBins(int x)   { gamma_bin = x; }
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
TH2D * GatePromptGammaData::GetGammaZ() { return GammaZ; }
TH2D * GatePromptGammaData::GetNgamma() { return Ngamma; }

TH2D * GatePromptGammaData::GetHEpEpgNormalized()          { return pHEpEpgNormalized; }
TH1D * GatePromptGammaData::GetHEp()                       { return pHEp; }
TH1D * GatePromptGammaData::GetHEpInelastic()              { return pHEpInelastic; }
TH1D * GatePromptGammaData::GetHEpSigmaInelastic()         { return pHEpSigmaInelastic; }
TH2D * GatePromptGammaData::GetHEpEpg()                    { return pHEpEpg; }
TH1D * GatePromptGammaData::GetHEpInelasticProducedGamma() { return pHEpInelasticProducedGamma; }
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GatePromptGammaData::ResetData()
{
  GammaZ->Reset();
  Ngamma->Reset();
  pHEpEpg->Reset();
  pHEpEpgNormalized->Reset();
  pHEpInelastic->Reset();
  pHEp->Reset();
  pHEpInelasticProducedGamma->Reset();
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GatePromptGammaData::SaveData()
{
  pTfile->Write();
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GatePromptGammaData::Read(std::string & filename)
{
  mFilename = filename;
  pTfile = new TFile(filename.c_str(),"READ");

  // How many material in this file
  TList * l = pTfile->GetListOfKeys();
  unsigned int n = l->GetSize();
  GateMessage("Actor", 1, "Reading " << n << " elements in " << filename << std::endl);
  std::string s;
  for(unsigned int i=0; i<n; i++) {
    s += std::string(l->At(i)->GetName())+std::string(" ");
  }
  GateMessage("Actor", 1, "Elements are : " << s << std::endl);

  // Prepare vector of histo
  const G4ElementTable & table = *G4Element::GetElementTable();
  unsigned int m = table.size();
  GateMessage("Actor", 1, "G4Elements table has " << m << " elements." << std::endl);

  GammaZList.resize(m);
  NgammaList.resize(m);
  pHEpEpgList.resize(m);
  pHEpEpgNormalizedList.resize(m);
  pHEpInelasticList.resize(m);
  pHEpList.resize(m);
  pHEpInelasticProducedGammaList.resize(m);
  pHEpSigmaInelasticList.resize(m);
  ElementIndexList.resize(m);
  std::fill(ElementIndexList.begin(), ElementIndexList.end(), false);

  for(unsigned int i=0; i<n; i++) {
    int index = -1;
    for(unsigned int e=0; e<table.size(); e++) {
      if (l->At(i)->GetName() == table[e]->GetName()) index = e;
    }
    if (index == -1) {
      GateMessage("Actor", 1, "Skipping " << l->At(i)->GetName()
                  << ", not used by any material (not in G4Elements table)." << std::endl);
      continue;
    }

    // Set current pointer
    const G4Element * elem = table[index];

    if (elem->GetZ() == 1) {
      GateMessage("Actor", 1, "Skipping Hydrogen (no prompt gamma)." << std::endl);
      continue; // proton (Hydrogen) skip
    }

    SetCurrentPointerForThisElement(elem);
    ElementIndexList[elem->GetIndex()] = true;
    std::string f = elem->GetName();
    TDirectory * dir = pTfile->GetDirectory(f.c_str());
    dir->cd();
    dir->GetObject("GammaZ", GammaZ);
    dir->GetObject("Ngamma", Ngamma);
    dir->GetObject("EpEpg", pHEpEpg);
    dir->GetObject("EpEpgNorm", pHEpEpgNormalized);
    dir->GetObject("EpInelastic", pHEpInelastic);
    dir->GetObject("Ep", pHEp);
    dir->GetObject("SigmaInelastic", pHEpSigmaInelastic);
    dir->GetObject("EpInelasticProducedGamma", pHEpInelasticProducedGamma);

    // Set the pointers in the lists (have been changed by GetObject)
    GammaZList[elem->GetIndex()] = GammaZ;
    NgammaList[elem->GetIndex()] = Ngamma;
    pHEpEpgNormalizedList[elem->GetIndex()] = pHEpEpgNormalized;
    pHEpEpgList[elem->GetIndex()] = pHEpEpg;
    pHEpInelasticList[elem->GetIndex()] = pHEpInelastic;
    pHEpList[elem->GetIndex()] = pHEp;
    pHEpSigmaInelasticList[elem->GetIndex()] = pHEpSigmaInelastic;
    pHEpInelasticProducedGammaList[elem->GetIndex()] = pHEpInelasticProducedGamma;

    SetProtonNbBins(GammaZ->GetXaxis()->GetNbins());
    SetGammaNbBins(GammaZ->GetYaxis()->GetNbins());

    SetProtonEMin(GammaZ->GetXaxis()->GetXmin());
    SetProtonEMax(GammaZ->GetXaxis()->GetXmax());
    SetGammaEMin(GammaZ->GetYaxis()->GetXmin());
    SetGammaEMax(GammaZ->GetYaxis()->GetXmax());

  }

  for(unsigned int i=0; i<m; i++) {
    if (ElementIndexList[i]) {
      GateMessage("Actor", 1, "Element " << table[i]->GetName()
                  << " is read." << std::endl);
    }
  }

}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void GatePromptGammaData::SetCurrentPointerForThisElement(const G4Element * elem)
{
  int i = elem->GetIndex();
  GammaZ = GammaZList[i];
  Ngamma = NgammaList[i];

  pHEpEpg = pHEpEpgList[i];
  pHEpEpgNormalized = pHEpEpgNormalizedList[i];
  pHEpInelastic = pHEpInelasticList[i];
  pHEp = pHEpList[i];
  pHEpSigmaInelastic = pHEpSigmaInelasticList[i];
  pHEpInelasticProducedGamma = pHEpInelasticProducedGammaList[i];
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// FIXME change the name of this function
void GatePromptGammaData::Initialize(std::string & filename, const G4Material * material)
{
  // Open the file in update mode, part will be overwriten, part will
  // be keep as is.
  mFilename = filename;
  pTfile = new TFile(filename.c_str(),"UPDATE");

  // Check if a directory for this material already exist
  std::string name = material->GetName();
  TDirectory * dir = pTfile->GetDirectory(name.c_str());
  if (dir == 0) {
    // create a subdirectory for the material in this file.
    dir = pTfile->mkdir(name.c_str());
    dir->cd();
  }
  else {
    // If already exist -> will be replaced.
    dir->cd();
    dir->Delete("*;*");
  }

  GammaZ = new TH2D("GammaZ","Gamma_Z as found in JMs paper.",
                               proton_bin, min_proton_energy/MeV, max_proton_energy/MeV,
                               gamma_bin, min_gamma_energy/MeV, max_gamma_energy/MeV);
  GammaZ->SetXTitle("E_{proton} [MeV]");
  GammaZ->SetYTitle("E_{gp} [MeV]");

  Ngamma = new TH2D("Ngamma","PG count per proton energy bin",
                     proton_bin, min_proton_energy/MeV, max_proton_energy/MeV,
                     gamma_bin, min_gamma_energy/MeV, max_gamma_energy/MeV);
  Ngamma->SetXTitle("E_{proton} [MeV]");
  Ngamma->SetYTitle("E_{gp} [MeV]");

  pHEpEpg = new TH2D("EpEpg","PG count",
                     proton_bin, min_proton_energy/MeV, max_proton_energy/MeV,
                     gamma_bin, min_gamma_energy/MeV, max_gamma_energy/MeV);
  pHEpEpg->SetXTitle("E_{proton} [MeV]");
  pHEpEpg->SetYTitle("E_{gp} [MeV]");

  pHEpEpgNormalized = new TH2D("EpEpgNorm","PG normalized by mean free path in [m]",
                               proton_bin, min_proton_energy/MeV, max_proton_energy/MeV,
                               gamma_bin, min_gamma_energy/MeV, max_gamma_energy/MeV);
  pHEpEpgNormalized->SetXTitle("E_{proton} [MeV]");
  pHEpEpgNormalized->SetYTitle("E_{gp} [MeV]");

  pHEpInelastic = new TH1D("EpInelastic","proton energy for each inelastic interaction",
                           proton_bin, min_proton_energy/MeV, max_proton_energy/MeV);
  pHEpInelastic->SetXTitle("E_{proton} [MeV]");

  pHEp = new TH1D("Ep","proton energy",proton_bin, min_proton_energy/MeV, max_proton_energy/MeV);
  pHEp->SetXTitle("E_{proton} [MeV]");

  pHEpSigmaInelastic = new TH1D("SigmaInelastic","Sigma inelastic Vs Ep",
                                proton_bin, min_proton_energy/MeV, max_proton_energy/MeV);
  pHEpSigmaInelastic->SetXTitle("E_{proton} [MeV]");

  pHEpInelasticProducedGamma = new TH1D("EpInelasticProducedGamma",
                                        "proton energy for each inelastic interaction if gamma production",
                                        proton_bin, min_proton_energy/MeV, max_proton_energy/MeV);
  pHEpInelasticProducedGamma->SetXTitle("E_{proton} [MeV]");

  ResetData();
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// FIXME change the name of this function
void GatePromptGammaData::InitializeMaterial()
{
  GateMessage("Actor", 1, "Create DB for used material. " << std::endl);
  const G4MaterialTable & matTable = *G4Material::GetMaterialTable();
  unsigned int n = G4Material::GetNumberOfMaterials();
  GateMessage("Actor", 1, "Number of materials : " << n << std::endl);

  mGammaEnergyHistoByMaterialByProtonEnergy.resize(n);
  GammaM.resize(n);
  NgammaM.resize(n);

  for(unsigned int i=0; i<n; i++) {
    bool stop = false;
    const G4Material * m = matTable[i];

    // Check material
    for(unsigned int e=0; e<m->GetNumberOfElements(); e++) {
      const G4Element * elem = m->GetElement(e);
      unsigned int elementIndex = elem->GetIndex();
      if (elem->GetZ() != 1) { // skip
        if (ElementIndexList[elementIndex] == false) {
          GateMessage("Actor", 1, "Skipping " << m->GetName()
                      << " because " << elem->GetName()
                      << " is missing in the DB." << std::endl);
          stop = true;
        }
      }
    }

    if (!stop) {
      GateMessage("Actor", 1, "Create DB for " << m->GetName()
                  << " (d = " << m->GetDensity()/(g/cm3)
                  << ")" << std::endl);
      mGammaEnergyHistoByMaterialByProtonEnergy[i].resize(proton_bin+1); // from [1 to n]

      //set mGammaEnergyHistoByMaterialByProtonEnergy
      for(unsigned int j=1; j<proton_bin+1; j++) {
        TH1D * h = new TH1D();
        h->SetBins(gamma_bin, min_gamma_energy, max_gamma_energy);

        // Loop over element
        for(unsigned int e=0; e<m->GetNumberOfElements(); e++) {
          const G4Element * elem = m->GetElement(e);
          double f = m->GetFractionVector()[e];

          if (elem->GetZ() != 1) { // if not Hydrogen.
            // (If hydrogen probability is zero)
            // Get histogram for the current bin
            SetCurrentPointerForThisElement(elem);
            TH1D * he = new TH1D(*pHEpEpgNormalized->ProjectionY("", j, j));

            // Scale it according to the fraction of this element in the material
            he->Scale(f);

            // Add it to the current total histo
            h->Add(he);
          }
        }
        mGammaEnergyHistoByMaterialByProtonEnergy[i][j] = h;
      }

      //Build GammaZ -> GammaM, EpEpg=Ngamma(z,E) -> Ngamma(m,E)
      TH2D * hgammam = new TH2D();
      TH2D * hngammam = new TH2D();
      hgammam->SetBins(proton_bin, min_proton_energy, max_proton_energy, gamma_bin, min_gamma_energy, max_gamma_energy); //same arrangement as GammaZ
      hngammam->SetBins(proton_bin, min_proton_energy, max_proton_energy, gamma_bin, min_gamma_energy, max_gamma_energy);
      // Loop over element
      for(unsigned int e=0; e<m->GetNumberOfElements(); e++) {
        const G4Element * elem = m->GetElement(e);
        double f = m->GetFractionVector()[e];

        if (elem->GetZ() != 1) { // if not Hydrogen.
          // (If hydrogen probability is zero)
          // Get histogram for the current bin
          SetCurrentPointerForThisElement(elem);
          //TH2D * hgammam2 = new TH2D *GammaZ->Clone();
          //TH2D * hngammam2 = new TH2D *Ngamma->Clone();
          TH2D * hngammam2 = (TH2D*) Ngamma->Clone();
          TH2D * hgammam2 = (TH2D*) GammaZ->Clone();

          // Scale it according to the fraction of this element in the material, multiplied with density ratio
          hgammam2->Scale(f * m->GetDensity() / (g / cm3) );// GammaZ=GammaZ/rho(Z), so dont need to divide by rho(Z)
          hngammam2->Scale(f * m->GetDensity() / (g / cm3) );
          DD(m->GetDensity()/e ); //!FIXME!!!!!!!!!!!!!!!!!!!!! THIS IS RHO(Z), NOT RHO(M)!!!!

          // Add it to the current total histo
          hgammam->Add(hgammam2);
          hngammam->Add(hngammam2);
        }
      }
      GammaM[i] = hgammam; //Now it's no longer modulo rho(Z), or rho(M)!!!
      NgammaM[i] = hngammam;
    }
  }
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
bool GatePromptGammaData::DataForMaterialExist(const int & materialIndex)
{
  if (mGammaEnergyHistoByMaterialByProtonEnergy[materialIndex].size() == 0) return false;
  return true;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
TH1D * GatePromptGammaData::GetGammaEnergySpectrum(const int & materialIndex,
                                                   const double & energy)
{
  // Check material
  if (!DataForMaterialExist(materialIndex)) {
    GateError("Error in GatePromptGammaData for TLE, the material " <<
              (*G4Material::GetMaterialTable())[materialIndex]->GetName()
              << " is not in the DB");
  }

  // Get the index of the energy bin
  int binX = pHEp->FindFixBin(energy);

  // Get the projected histogram of the material
  TH1D * h = mGammaEnergyHistoByMaterialByProtonEnergy[materialIndex][binX];

  return h;
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
TH1D* GatePromptGammaData::GetGammaMForGammaBin(const int & materialIndex,const int & energyBin)
{
  // Check material
  if (!DataForMaterialExist(materialIndex)) {
    GateError("Error in GatePromptGammaData for TLE, the material " <<
              (*G4Material::GetMaterialTable())[materialIndex]->GetName()
              << " is not in the DB");
  }

  // Get the projected histogram of the material
  // projectionX: project on X, from biny to biny2. And indeed, on X-axis are protons.
  TH1D * h = new TH1D (*GammaM[materialIndex]->ProjectionX("", energyBin, energyBin));

  return h;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
TH1D* GatePromptGammaData::GetNgammaMForGammaBin(const int & materialIndex,
                                                   const int & energyBin)
{
  // Check material
  if (!DataForMaterialExist(materialIndex)) {
    GateError("Error in GatePromptGammaData for TLE, the material " <<
              (*G4Material::GetMaterialTable())[materialIndex]->GetName()
              << " is not in the DB");
  }

  // Get the projected histogram of the material
  TH1D * h = new TH1D (*NgammaM[materialIndex]->ProjectionX("", energyBin, energyBin));

  return h;
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
TH2D* GatePromptGammaData::GetNgammaM(const int & materialIndex)
{
  return NgammaM[materialIndex];
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
TH2D* GatePromptGammaData::GetGammaM(const int & materialIndex)
{
  return GammaM[materialIndex];
}
//-----------------------------------------------------------------------------

